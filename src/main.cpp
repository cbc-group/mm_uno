#include <mcp49xx.h>

#include <Arduino.h>
#include <inttypes.h>
#include <SPI.h>


/*
 * DAC pinout
 */
#define CS_PIN      52
#define LDAC_PIN    -1
mcp49xx dac(mcp49xx::mcp4922, CS_PIN, LDAC_PIN);
volatile byte armed = false;


/*
 * camera pinout
 */
#define TRIGGER     11


/*
 * waveform parameters
 */
#define N_STEPS     4096
#define DELTA_T     70


/*
 * states
 */
enum states_t {
    WAIT_TRIG = 1,
    ACQUIRING = 2, 
    FINISH = 3
};
volatile states_t state = WAIT_TRIG;


/*
 * interrupt routines
 */
void camera_fired() {
    armed = true;
}


void setup() {
    // DAC initialized
    SPI.begin(CS_PIN);
    
    dac.setGain(1);
    // set default value
    dac.output(0);

    armed = false;
    pinMode(TRIGGER, INPUT);
    attachInterrupt(digitalPinToInterrupt(TRIGGER), camera_fired, RISING);
} 

// primary loop, state machine control
void loop() {
    static volatile byte direction = true;
    static volatile uint16_t index = 0;
    
    if (armed) {
        // apply output
        dac.outputA(index);
        
        // determine direction
        if (index == 0 && !direction) {
            direction = true;
            armed = false;
        } else if (index == 4095 && direction) {
            direction = false;
            armed = false;
        }

        // apply increment/decrement
        if (direction) {
            index++;
        } else {
            index--;
        }
    }

    delayMicroseconds(15);
}


