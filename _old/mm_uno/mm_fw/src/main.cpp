#include "mcp49xx.h"


/*
 * DAC pinout
 */
#define CS_PIN      5
#define SCK_PIN     13
#define SDI_PIN     11
#define LDAC_PIN    6
mcp49xx dac(mcp49xx::mcp4921, CS_PIN, SCK_PIN, SDI_PIN, LDAC_PIN);
volatile byte dac_armed = false;


/*
 * camera pinout
 */
#define TRIGGER     2


/*
 * waveform parameters
 */
#define N_STEPS     4096
#define DELTA_T     70
volatile unsigned short value = 0;
volatile byte direction = false;


/*
 * interrupt routines
 */
void camera_fired() {
    dac_armed = true;
}


void setup() {
    dac.setGain(1);

    // set default value
    dac.output(0);
    dac.latch();

    pinMode(TRIGGER, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(TRIGGER), camera_fired, RISING);
}

void loop()
{
    if (dac_armed) {
        noInterrupts();
        dac_armed = false;

        for (unsigned short i = 0; i < N_STEPS; i++) {
            if (direction) {
                value++;
            } else {
                value--;
            }
            dac.output(value);
            dac.latch();

            delayMicroseconds(DELTA_T);
        }
        // reverse direction
        direction = !direction;

        interrupts();
    }
}
