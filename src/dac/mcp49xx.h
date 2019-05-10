#ifndef _MCP49xx_H
#define _MCP49xx_H

#include <Arduino.h>
#include <inttypes.h>

// Microchip MCP4901/MCP4911/MCP4921/MCP4902/MCP4912/MCP4922 DAC driver
// Thomas Backman, 2012

class mcp49xx
{
  public:
    // These are the DAC models we support
    enum Model
    {
        MCP4901 = 1, /* single, 8-bit */
        MCP4911,     /* single, 10-bit */
        MCP4921,     /* single, 12-bit */
        MCP4902,     /* dual, 8-bit */
        MCP4912,     /* dual, 10-bit */
        MCP4922      /* dual, 12-bit */
    };

    enum Channel
    {
        CHANNEL_A = 0,
        CHANNEL_B = 1
    };

    mcp49xx(Model _model, int _cs_pin, int _sck_pin, int _sdi_pin, int _ldac_pin = -1);
    void setBuffer(boolean _buffer);
    boolean setGain(int _gain);
    void shutdown(void);

    /* All of these call _output() internally */
    void output(unsigned short _out);
    void outputA(unsigned short _out); /* same as output(), but having A/B makes more sense for dual DACs */
    void outputB(unsigned short _out);
    void output2(unsigned short _out, unsigned short _out2); // For MCP49x2

    void latch(void); // Actually change the output, if the LDAC pin isn't shorted to ground
    boolean setAutomaticallyLatchDual(bool _latch);

  private:
    void _output(unsigned short _out, Channel _chan);
    void _transfer(byte msb, byte lsb);
    int cs_pin;
    int sck_pin;
    int sdi_pin;
    int ldac_pin;
    int bitwidth;
    boolean bufferVref;
    boolean gain2x;                 /* false -> 1x, true -> 2x */
    boolean automaticallyLatchDual; /* call latch() automatically after output2() has been called? */
};

#endif
