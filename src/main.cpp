#include <mcp49xx.h>

#include <Arduino.h>
//#include <SPI.h>


/*
 * DAC pinout
 */
#define CS_PIN      52
#define LDAC_PIN    7
mcp49xx dac(mcp49xx::mcp4922, CS_PIN);
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

    pinMode(11, INPUT);
    //attachInterrupt(digitalPinToInterrupt(TRIGGER), camera_fired, RISING);

    //DEBUG
    pinMode(12, OUTPUT);
    digitalWrite(12, 0);
}

void loop()
{
    dac_armed = digitalRead(11);
    digitalWrite(12, !dac_armed);

    for (uint16_t i = 0; i < 4096; i++) {
        dac.output2(i, (4096-i)-1);
    }

    /*
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
    */
}
