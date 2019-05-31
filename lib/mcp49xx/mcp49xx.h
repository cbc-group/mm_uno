#ifndef __MCP49XX_H__
#define __MCP49XX_H__

#include "simba.h"

/* DAC model. */
#define MCP4901             0x01
#define MCP4911             0x02
#define MCP4921             0x04
#define MCP4902             0x11
#define MCP4912             0x12
#define MCP4922             0x14

/* Input buffer control. */
#define MCP49XX_UNBUFFERED  0x01
#define MCP49XX_BUFFERED    0x00

/* Output gain selection. */
#define MCP49XX_GAIN_1X     0x01
#define MCP49XX_GAIN_2X     0x00

/* Driver data structure. */
struct mcp49xx_driver_t {
    struct spi_driver_t spi;
    THRD_STACK(stack, 64); // TODO is this really necessary?
};

/**
 * Initialize given driver object.
 * 
 * @param[out] self_p Driver object to initialize.
 * @param[in] spi_p SPI driver to use.
 * @param[in] cs_p SPI chip select pin.
 * 
 */
int mcp49xx_init(struct mcp49xx_driver_t *self_p,
                 struct spi_device_t *spi_p,
                 struct pin_device_t *cs_p,
                 int model, 
                 int buffer,
                 int gain);

/**
 * Starts the DAC device using given driver object.
 */
int mcp49xx_start(struct mcp49xx_driver_t *self_p);

/**
 * Stops the DAC device reference by driver object.
 */
int mcp49xx_stop(struct mcp49xx_driver_t *self_p);

/**
 * Write new analog value.
 */
ssize_t mcp49xx_write(struct mcp49xx_driver_t *self_p,
                      uint16_t value);





#include <Arduino.h>
#include <inttypes.h>

// Microchip mcp4901/mcp4911/mcp4921/mcp4902/mcp4912/mcp4922 DAC driver
// Thomas Backman, 2012

class mcp49xx
{
  public:
    // These are the DAC models we support
    enum Model
    {
        mcp4901 = 1, /* single, 8-bit */
        mcp4911,     /* single, 10-bit */
        mcp4921,     /* single, 12-bit */
        mcp4902,     /* dual, 8-bit */
        mcp4912,     /* dual, 10-bit */
        mcp4922      /* dual, 12-bit */
    };

    enum Channel
    {
        CHANNEL_A = 0,
        CHANNEL_B = 1
    };

    mcp49xx(Model _model, int _cs_pin, int _ldac_pin = -1);
    void setBuffer(boolean _buffer);
    boolean setGain(int _gain);
    void shutdown(void);

    /* All of these call _output() internally */
    void output(unsigned short _out);
    void outputA(unsigned short _out); /* same as output(), but having A/B makes more sense for dual DACs */
    void outputB(unsigned short _out);
    void output2(unsigned short _out, unsigned short _out2); // For mcp49x2

    void latch(void); // Actually change the output, if the LDAC pin isn't shorted to ground
    boolean setAutomaticallyLatchDual(boolean _latch);

  private:
    void _output(unsigned short _out, Channel _chan);
    void _transfer(uint16_t data);
    int cs_pin;
    int ldac_pin;
    int bitwidth;
    boolean bufferVref;
    boolean gain2x;                 /* false -> 1x, true -> 2x */
    boolean automaticallyLatchDual; /* call latch() automatically after output2() has been called? */
};

#endif
