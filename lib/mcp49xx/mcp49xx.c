
#include "simba.h"

/* mcp49xx control frame structure. */
struct spi_frame_t {
    uint8_t ab : 1;     // a/b channel selection bit
    uint8_t buf : 1;    // input buffer control bit
    uint8_t ga : 1;     // output gain selection bit
    uint8_t shdn : 1;   // output shutdown control bit
    uint16_t data : 12;
} PACKED;

int mcp49xx_init(struct mcp49xx_driver_t *self_p,
                 struct spi_device_t *spi_p,
                 struct pin_device_t *cs_p,
                 int buffer,
                 int gain)
{
    ASSERTN(self_p != NULL, EINVAL);
    ASSERTN(spi_p != NULL, EINVAL);
    ASSERTN(cs_p != NULL, EINVAL);

    spi_init(&self_p->spi,
             spi_p,
             cs_p,
             SPI_MODE_MASTER,
             SPI_SPEED_1MBPS,
             0,
             0);
    
    return (0);
}

int mcp49xx_start(struct mcp49xx_driver_t *self_p) 
{
    ASSERTN(self_p != NULL, EINVAL);

    spi_start(&self_p->spi);
}

int mcp49xx_stop(struct mcp49xx_driver_t *self_p)
{
    ASSERTN(self_p != NULL, EINVAL);
}



/*
 * Modified by Andy @ Academia Sinica BCChen lab 2018/1/31
 * This modified library is using soft SPI to work with Micromanager Arduino firmware
 * 
 * Microchip mcp4901 / mcp4911 / mcp4921 / mcp4902 / mcp4912 / mcp4922 8/10/12-bit DAC driver
 * Thomas Backman, 2012-07 (made a proper library 2011-07-30, 3 weeks after initial)
 * serenity@exscape.org
 * Support for mcp49x2 (mcp4902, mcp4912, mcp4922) added 2012-08-30,
 * with a patch and testing from Jonas Gruska
 
 * Code license: BSD/MIT. Whatever; I *prefer* to be credited,
 * and you're *not* allowed to claim you wrote this from scratch.
 * Apart from that, do whatever you feel like. Commercial projects,
 * code modifications, etc.

 Pins used: 
 * Arduino pin 11 (for Uno; for Mega: 51) to device SDI (pin 4) - fixed pin
 * Arduino pin 13 (for Uno; for Mega: 52) to device SCK (pin 3) - fixed pin
 * Any digital pin to device LDAC (DAC pin 5)  (except with PortWrite, see README)
 * Any digital pin to device CS   (DAC pin 2)  (as above)
 *
 * Other DAC wirings:  
 * Pin 1: VDD, to +5 V
 * Pin 5: LDAC, either to an Arduino pin, or ground to update vout automatically
 * Pin 6: VREF, to +5 V (or some other reference voltage 0 < VREF <= VDD)
 * Pin 7: VSS, to ground
 * Pin 8: vout
 * (Pin 9, for the DFN package only: VSS)

 * Only tested on mcp4901 (8-bit) and mcp4922 (dual 12-bit), but it should work on the others as well.
 * Tested on an Arduino Uno R3.
 */

#include "mcp49xx.h"
#include <SPI.h>

mcp49xx::mcp49xx(mcp49xx::Model _model, int _cs_pin, int _ldac_pin)
    : bufferVref(false), gain2x(false), automaticallyLatchDual(true)
{
    this->cs_pin = _cs_pin;
    this->ldac_pin = _ldac_pin;

    /* 
  * mcp49x1 models are single DACs, while mcp49x2 are dual.
  * Apart from that, only the bit width differ between the models.
  */
    switch (_model)
    {
    case mcp4901:
    case mcp4902:
        bitwidth = 8;
        break;
    case mcp4911:
    case mcp4912:
        bitwidth = 10;
        break;
    case mcp4921:
    case mcp4922:
        bitwidth = 12;
        break;
    default:
        bitwidth = 0;
    }
    
    // NOTE must initialize externally in setup()
    //SPI.begin(cs_pin);

    // un-latch the output
    if (ldac_pin >= 0) {
        pinMode(ldac_pin, OUTPUT);
        digitalWrite(ldac_pin, HIGH);
    }
}

void mcp49xx::setBuffer(boolean _buffer)
{
    this->bufferVref = _buffer;
}

// Only relevant for the mcp49x2 dual DACs.
// If set, calling output2() will pull the LDAC pin low automatically,
// which causes the output to change.
// Not required if the LDAC pin is shorted to ground, however in that case,
// there will be a delay between the updating of channel A and channel B.
// If sync is desired, wire the LDAC pin to the Arduino and set this to true.
boolean mcp49xx::setAutomaticallyLatchDual(bool _latch)
{
    automaticallyLatchDual = _latch;
    return _latch;
}

// Sets the gain. These DACs support 1x and 2x gain.
// vout = x/2^n * gain * VREF, where x = the argument to out(), n = number of DAC bits
// Example: with 1x gain, set(100) on a 8-bit (256-step) DAC would give
// an output voltage of 100/256 * VREF, while a gain of 2x would give
// vout = 100/256 * VREF * 2
boolean mcp49xx::setGain(int _gain)
{
    if (_gain == 1)
    {
        gain2x = false;
        return true;
    }
    else if (_gain == 2)
    {
        gain2x = false; // set only gain1x
        return true;
    }
    else
        return false; // DAC only supports 1x and 2x
}

// Shuts the DAC down. Shutdown current is about 1/50 (typical) of active mode current.
// My measurements say ~160-180 µA active (unloaded vout), ~3.5 µA shutdown.
// Time to settle on an output value increases from ~4.5 µs to ~10 µs, though (according to the datasheet).
void mcp49xx::shutdown(void)
{
    // Sending all zeroes should work, too, but I'm unsure whether there might be a switching time
    // between buffer and gain modes, so we'll send them so that they have the same value once we
    // exit shutdown.
    unsigned short out = (bufferVref << 14) | ((!(gain2x)) << 13); // gain == 0 means 2x, so we need to invert it
    _transfer(out);
}

// Private function.
// Called by the output* set of functions.
void mcp49xx::_output(unsigned short data, Channel chan)
{
    // Truncate the unused bits to fit the 8/10/12 bits the DAC accepts
    if (this->bitwidth == 12)
        data &= 0xfff;
    else if (this->bitwidth == 10)
        data &= 0x3ff;
    else if (this->bitwidth == 8)
        data &= 0xff;

    // bit 15: 0 for DAC A, 1 for DAC B. (Always 0 for mcp49x1.)
    // bit 14: buffer VREF?
    // bit 13: gain bit; 0 for 1x gain, 1 for 2x (thus we NOT the variable)
    // bit 12: shutdown bit. 1 for active operation
    // bits 11 through 0: data
    uint16_t out = (chan << 15) | (this->bufferVref << 14) | ((!this->gain2x) << 13) | (1 << 12) | (data << (12 - this->bitwidth));
    // Send the command and data bits
    _transfer(out);
}

// For mcp49x1
void mcp49xx::output(unsigned short data)
{
    this->_output(data, CHANNEL_A);
}

// For mcp49x2
void mcp49xx::outputA(unsigned short data)
{
    this->_output(data, CHANNEL_A);
}

// For mcp49x2
void mcp49xx::outputB(unsigned short data)
{
    this->_output(data, CHANNEL_B);
}

// mcp49x2 (dual DAC) only.
// Send a set of new values for the DAC to output in a single function call
void mcp49xx::output2(unsigned short data_A, unsigned short data_B)
{
    this->_output(data_A, CHANNEL_A);
    this->_output(data_B, CHANNEL_B);

    // Update the output, if desired.
    // The reason this is only in the dual-output version is simple: it's mostly useless
    // for the single-output version, as it would make more sense to tie the \LDAC pin
    // to ground, or do it manually. However, there should be a single-call method
    // to update *both* channels in sync, which wouldn't be possible with multiple
    // separate DACs (for which there is latch()).
    if (automaticallyLatchDual)
    {
        this->latch();
    }
}

// These DACs have a function where you can change multiple DACs at the same
// time: you call output() "sequentially", one DAC at a time, and *then*,
// when they've all received the output data, pull the LDAC pin low on
// all DACs at once. This function pulls the LDAC pin low for long enough
// for the DAC(s) to change the output.
// If this function is undesired, you can simply tie the LDAC pin to ground.
// When tied to ground, you need *NOT* call this function!
void mcp49xx::latch(void)
{
    // The datasheet says CS must return high for 40+ ns before this function
    // is called: no problems, that'd be taken care of automatically, as one
    // clock cycle at 16 MHz is longer... and there'll be a delay of multiple.
    if (ldac_pin < 0)
        return;

    // We then need to hold LDAC low for at least 100 ns, i.e ~2 clock cycles.
    // This takes far, FAR longer than the above despite no NOP; digitalWrite
    // is SLOW! For comparison: the above takes 180 ns, this takes... 3.8 us,
    // or 3800 ns, 21 times as long - WITHOUT having a delay in there!
    digitalWrite(ldac_pin, LOW);
    digitalWrite(ldac_pin, HIGH);
}

void mcp49xx::_transfer(uint16_t data)
{
    SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
    SPI.transfer16(cs_pin, data); 
    SPI.endTransaction();
}
