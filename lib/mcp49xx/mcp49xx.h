#ifndef _MCP49XX_H
#define _MCP49XX_H

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
