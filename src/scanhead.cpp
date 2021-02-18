/*
 * scanhead.cpp
 * Kaveh Pezeshki 1/18/2021
 * Tracks and controls position of STM scan tip
 */


#include "Arduino.h"
#include "scanhead.h"

#include "SPI.h"



ScanHead::ScanHead():
    stepper0(60, stepper0_pins.A, stepper0_pins.C, stepper0_pins.B, stepper0_pins.D),
    stepper1(60, stepper1_pins.A, stepper1_pins.C, stepper1_pins.B, stepper1_pins.D),
    stepper2(60, stepper2_pins.A, stepper2_pins.C, stepper2_pins.B, stepper2_pins.D)

{
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
    delay(1);

    // Setting local variables
    xpos = 0;
    ypos = 0;
    zpos = 0;
    zposStepper = 0;
    current= 0;

    // Setting piezo to zero
    setPiezo(piezo.chX_P, 0);
    setPiezo(piezo.chX_N, 0);
    setPiezo(piezo.chY_P, 0);
    setPiezo(piezo.chY_N, 0);

    // Setting sample piezo
    setPiezo(piezo.samplePad, 37500); // pad to about -0.5V (empirical)

    delay(1);
}

int ScanHead::setPositionStep(int xpos_set, int ypos_set, int zcurr_set) {
    /*!
     * \brief Provides PID control over scan head position via piezos. Call until expected return code. Will retract steppers on overcurrent
     * @param xpos_set Desired X position: integer from -32768 to 32768
     * @param ypos_set Desired Y position: integer from -32768 to 32768
     * @param zcurr_set Desired Z current in pA. -1 will provide no height control, -2 will retract.
     * @return 0 if transverse position not yet attained, -1 if position unachievable, -2 if overcurrent, 1 if obtained.
     */

    /*
     * Implementation notes:
     * - four channels: X+, X-, Y+, Y-. Applying a voltage to all channels causes z-displacement
     * - Z-displacement is in the negative direction. Invert all channels to get true z
     * - centered around zero - so negative voltage applied for < 2^16/2, positive for > 2^16/2, ~0 = 2^16/2
     */

    // really shitty p-control

    status = 0;

    int xStepIncrement = (xpos_set-xpos)*pidTransverseP;
    int yStepIncrement = (ypos_set-ypos)*pidTransverseP;
    int zStepIncrement = 0;

    //Serial.println();

    //Serial.print("xpos ");
    //Serial.print(xpos);
    //Serial.print(" xpos set ");
    //Serial.println(xpos_set);

    //Serial.print("ypos ");
    //Serial.print(ypos);
    //Serial.print(" ypos set ");
    //Serial.println(ypos_set);

    //Serial.print("xStepIncrement ");
    //Serial.println(xStepIncrement);
    //Serial.print("yStepIncrement ");
    //Serial.println(xStepIncrement);

    current = fetchCurrent();

    if (zcurr_set == -1) zStepIncrement = 0;
    else if (zcurr_set == -2) zStepIncrement = -1 * maxZStep;
    else zStepIncrement = (zcurr_set-current)*pidZP;

    //Serial.print("zcurr_set ");
    //Serial.print(zcurr_set);
    //Serial.print(" current ");
    //Serial.print(current);
    //Serial.print(" zStepIncrement ");

    // checking for overcurrent. If overcurrent, retract and return
    //
    if (current > overCurrent) {
        status = 3;
        moveStepper(-50, 4096);
        return -2;
    }

    // checking step size, setting step to max size if necessary

    if (abs(xStepIncrement) > maxTransverseStep) xStepIncrement = maxTransverseStep * (xStepIncrement/abs(xStepIncrement));
    if (abs(yStepIncrement) > maxTransverseStep) yStepIncrement = maxTransverseStep * (yStepIncrement/abs(yStepIncrement));
    if (abs(zStepIncrement) > maxTransverseStep) zStepIncrement = maxZStep * (zStepIncrement/abs(zStepIncrement));

    // applying step

    xpos += xStepIncrement;
    ypos += yStepIncrement;
    zpos += zStepIncrement;

    Serial.println(zpos);

    //Serial.print("new xpos ");
    //Serial.println(xpos);

    //Serial.print("new ypos ");
    //Serial.println(ypos);

    //Serial.print("new zpos ");
    //Serial.println(zpos);


    int chX_P = maxPiezo/2 + -zpos + xpos;
    int chX_N = maxPiezo/2 + -zpos - xpos;
    int chY_P = maxPiezo/2 + -zpos + ypos;
    int chY_N = maxPiezo/2 + -zpos - ypos;

    //Serial.print("chX_P ");
    //Serial.println(chX_P);
    //Serial.print("chX_N ");
    //Serial.println(chX_N);
    //Serial.print("chY_P ");
    //Serial.println(chY_P);
    //Serial.print("chY_N ");
    //Serial.println(chY_N);

    // checking bounds

    bool exceeded_bounds = false;

    if (chX_P > maxPiezo) {
        exceeded_bounds = true;
        chX_P = maxPiezo;
    }
    else if (chX_P < minPiezo) {
        exceeded_bounds = true;
        chX_P = minPiezo;
    }

    if (chX_N > maxPiezo) {
        exceeded_bounds = true;
        chX_N = maxPiezo;
    }
    else if (chX_N < minPiezo) {
        exceeded_bounds = true;
        chX_N = minPiezo;
    }

    if (chY_P > maxPiezo) {
        exceeded_bounds = true;
        chY_P = maxPiezo;
    }
    else if (chY_P < minPiezo) {
        exceeded_bounds = true;
        chY_P = minPiezo;
    }

    if (chY_N > maxPiezo) {
        exceeded_bounds = true;
        chY_N = maxPiezo;
    }
    else if (chY_N < minPiezo) {
        exceeded_bounds = true;
        chY_N = minPiezo;
    }

    // writing piezos

    setPiezo(piezo.chX_P, chX_P);
    setPiezo(piezo.chX_N, chX_N);
    setPiezo(piezo.chY_P, chY_P);
    setPiezo(piezo.chY_N, chY_N);

    delayMicroseconds(500);

    if (exceeded_bounds == true) {
        //Serial.println("exceeded bounds!");
        return -1;
    }
    else if (xpos == xpos_set && ypos == ypos_set) return 1;
    else return 0;

}

void ScanHead::testScanHeadPosition(int numsteps, int stepsize) {
    int x_start = -1*numsteps/2;
    int x_end   = numsteps/2;

    int y_start = -1*numsteps/2;
    int y_end   = numsteps/2;

    for (int xpos_set = x_start; xpos_set < x_end; xpos_set += stepsize) {
        for (int ypos_set = y_start; ypos_set < y_end; ypos_set += stepsize) {
            int status = 0;
            while (status != 1) {
                status = setPositionStep(ypos_set, ypos_set, -1); // just both for now, rather than doing a raster
                //Serial.print("xpos: ");
                //Serial.print(xpos_set);
                //Serial.print(" ypos: ");
                //Serial.print(ypos_set);
                //Serial.print(" return ");
                //Serial.println(status);
            }
        }
    }
}

void ScanHead::moveStepper(int steps, int stepRate) {
    /*!
     * \brief Moves steppers steps at stepRate steps per second. This is blocking!
     * @param steps Number of steps to move
     * @param stepRate Number of steps per second to increment
     */

    if (stepRate == 0) return;

    //Serial.print("moving with steprate steps ");
    //Serial.print(stepRate);
    //Serial.print(" ");
    //Serial.println(steps);

    status = 1;
    stepper0.setSpeed(abs(stepRate));
    stepper1.setSpeed(abs(stepRate));
    stepper2.setSpeed(abs(stepRate));

    // right now, we're stepping each stepper sequentially. This shouldn't matter - the steppers are approx. equidistant from the sample

    if (stepRate < 0) steps *= -1;

    stepper0.step(steps);
    stepper1.step(steps);
    stepper2.step(steps);

    zposStepper += steps;

}

int ScanHead::autoApproachStep(int zcurr_set) {
    /*!
     * \brief Automatically advances Z until surface is detected. Performs one 'step' iteration. Ensure z-position is zeroed before approach
     * @return 0 if surface not yet detected, 1 otherwise
     */

    setpoint = zcurr_set;

    int approachStatus = 0;

    // first, fully retract the scan head

    while (approachStatus == 0 or approachStatus == 1) {
        approachStatus = setPositionStep(0,0,-2);
    }

    // next, move the stepper a few steps forward but less than the maximum scan range

    moveStepper(3, 100);
    approachStatus = 0;

    // then, approach with piezos

    while (approachStatus == 0 or approachStatus == 1) {
        //Serial.println("loop step");
        approachStatus = setPositionStep(0,0,zcurr_set);
        current = fetchCurrent();
        //if (approachStatus != 0 and approachStatus != 1) {
        //    Serial.print("could not approach, returned status ");
        //    Serial.println(approachStatus);
        //    break;
        //}
        if (current > zcurr_set) {
           return 1;
        }
    }

    // current problem - seem to start from 0,0,0, then we index down, then things break. We want to fully retract, then extrude for each step. Right now, there's no way to retract! cuz its dumb

    return 0;
}

int ScanHead::fetchCurrent() {
    /*!
     * \brief Samples current and converts to pA
     * @return current in pA
     */

    int sumVal = 0;
    int numSamples = 5;
    for (int i = 0; i < numSamples; i++) {
        digitalWrite(tia.cs, LOW);
        delayMicroseconds(1);
        digitalWrite(tia.cs, HIGH);
        delayMicroseconds(1);
        digitalWrite(tia.cs, LOW);
        int16_t receivedVal_high = (int16_t) SPI1.transfer(0xff);
        int16_t receivedVal_low  = (int16_t) SPI1.transfer(0xff);
        delayMicroseconds(1);
        digitalWrite(tia.cs, HIGH);
        delayMicroseconds(1);
        digitalWrite(tia.cs, LOW);
        int receivedVal = receivedVal_high << 8 | receivedVal_low;
        sumVal += receivedVal;
    }

    current = tiaToCurrent(sumVal / numSamples); 

    return current; // might bias results to lower val due to rounding err, but we're ok with this

}

int ScanHead::currentToTia(int currentpA) {
    /*!
     * \brief converts physical current value (in pA) to equivalent TIA reading, assuming 100M TI gain
     * @param current_pa current in pA
     * @return raw TIA current values
     */

    return (int) ( 65536.0 / (3.3*10000) * (float) currentpA + calibratedNoCurrent);
}

int ScanHead::tiaToCurrent(int currentTIA) {
    /*!
     * \brief converts raw TIA reading to physical current value (in pA), assuming 100M TI gain
     * @param current_tia raw TIA current value
     * @return current in pA
     */

    return (int) ((float)currentTIA - calibratedNoCurrent) * 3.3 / 65536.0 * 10000.0;
}

void ScanHead::setPiezo(int channel, int value) {
    /*!
     * \brief sets piezo channel to raw value
     * @param channel Raw channel (0-7) to set on TIA
     * @param value Raw 16-bit value to set to ADC
     */

  status = 2;

  unsigned int dacMSB = highByte(value);
  unsigned int dacLSB = lowByte(value);

  byte command = 0b00000011;
  byte addrD1  = lowByte(channel) << 4 | dacMSB >> 4;
  byte addrD2  = dacMSB << 4 | dacLSB >> 4;
  byte addrD3  = dacLSB << 4;

  digitalWrite(piezo.cs, LOW);
  SPI.transfer(command);
  SPI.transfer(addrD1);
  SPI.transfer(addrD2);
  SPI.transfer(addrD3);
  digitalWrite(piezo.cs, HIGH);
}

int ScanHead::scanOneAxis(int *currents, int *zpos, int size, bool direction, bool heightControl) {
    /*!
     * \brief scans size piezo LSBs across X axis with optional height control. Writes z positions and currents to arrays
     * @param *currents Pointer to size long array to store currents
     * @param *zpos Pointer to size long array to store z positions
     * @param size number of piezo LSBs to scan over
     * @param direction true to scan in +x, false to scan in -x
     * @param heightControl true if height control enabled, false otherwise
     */

    int xStarting = xpos;
    int xEnding   = xpos-size;
    if (direction) xEnding = xpos+size;

    int xTarget = xStarting;

    int setCurrent = setpoint;
    if (!heightControl) setCurrent = -1; // no height control if -1 passed to setPositionStep

    int numSteps = 0;

    while (xpos != xEnding) {

        int setPositionStatus = setPositionStep(xTarget, ypos, setCurrent);
        while (setPositionStatus == 0) setPositionStatus = setPositionStep(xTarget, ypos, setCurrent);

        currents[numSteps] = current;
        zpos[numSteps] = zpos;

        if (setPositionStatus != 1) return setPositionStatus;

        if (direction) xTarget += 1;
        else xTarget -= 1;
        numSteps += 1;
    }

    return 0;

}
