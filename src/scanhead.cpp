/*
 * scanhead.cpp
 * Kaveh Pezeshki 1/18/2021
 * Tracks and controls position of STM scan tip
 */


#include "Arduino.h"
#include "scanhead.h"

#include "SPI.h"
#include "Stepper.h"


Stepper stepper0(64, stepper0_pins.A, stepper0_pins.C, stepper0_pins.B, stepper0_pins.D);
Stepper stepper1(64, stepper1_pins.A, stepper1_pins.C, stepper1_pins.B, stepper1_pins.D);
Stepper stepper2(64, stepper2_pins.A, stepper2_pins.C, stepper2_pins.B, stepper2_pins.D);


ScanHead::ScanHead() {
    // Setting up relevant pins

    // TIA
    pinMode(tia.cs, OUTPUT);
    pinMode(tia.din, OUTPUT);
    digitalWrite(tia.din, HIGH);
    SPI1.setMISO(tia.miso);
    SPI1.begin();
    SPI1.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE3));
    delay(10);

    // Piezo
    pinMode(piezo.cs, OUTPUT);
    SPI.begin();
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
    // enabling all outputs
    digitalWrite(piezo.cs, LOW);
    SPI.transfer(0b00000100);
    SPI.transfer(0b00000000);
    SPI.transfer(0b00000000);
    SPI.transfer(0b11111111);
    digitalWrite(piezo.cs, HIGH);
    // setting int reference
    digitalWrite(piezo.cs, LOW);
    SPI.transfer(0b00001000);
    SPI.transfer(0b00000000);
    SPI.transfer(0b00000000);
    SPI.transfer(0b00000001);
    digitalWrite(piezo.cs, HIGH);
    delay(10);


    // Setting local variables
    xpos = 0;
    ypos = 0;
    zpos = 0;
}

int setPositionStep(int xpos, int ypos, int zpos) {
    /*!
     * \brief Provides PID control over scan head position via piezos. Call until expected return code
     * @param xpos Desired X position: integer from -32768 to 32768
     * @param ypos Desired Y position: integer from -32768 to 32768
     * @param zpos Desired Z position: integer from -32768 to 32768
     * @return 0 if position not yet attained, 1 otherwise
     */

    return 0;
}


void moveStepper(int steps, int stepRate) {
    /*!
     * \brief Moves steppers steps at stepRate steps per second. This is blocking!
     * @param steps Number of steps to move
     * @param stepRate Number of steps per second to increment
     */

    // 4096 steps per revolution


}

int autoApproachStep() {
    /*!
     * \brief Automatically advances Z until surface is detected. Performs one 'step' iteration
     * @return 0 if surface not yet detected, 1 otherwise
     */

    return 0;
}

int fetchCurrent() {
    /*!
     * \brief Samples current and converts to pA
     * @return current in pA
     */

    return 0;
}

void moveSingleStepper(int stepper) {
    /*!
     * \brief Increments stepper one step
     * @param stepper Stepper to increment, int 0,1,2
     */
}
