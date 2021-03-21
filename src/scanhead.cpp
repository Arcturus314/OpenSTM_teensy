/*
 * scanhead.cpp
 * Kaveh Pezeshki 1/18/2021
 * Tracks and controls position of STM scan tip
 */


#include "Arduino.h"
#include "scanhead.h"
#include <CircularBuffer.h>

#include "SPI.h"



ScanHead::ScanHead():
    stepper0(60, stepper0_pins.A, stepper0_pins.C, stepper0_pins.B, stepper0_pins.D),
    stepper1(60, stepper1_pins.A, stepper1_pins.C, stepper1_pins.B, stepper1_pins.D),
    stepper2(60, stepper2_pins.A, stepper2_pins.C, stepper2_pins.B, stepper2_pins.D),
    tiafilter()

{
    // Setting up relevant pins

    // TIA
    pinMode(tia.cs, OUTPUT);
    pinMode(tia.din, OUTPUT);
    digitalWrite(tia.din, HIGH);
    SPI1.setMISO(tia.miso);
    SPI1.begin();
    SPI1.beginTransaction(SPISettings(300000, MSBFIRST, SPI_MODE3));
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
    current = 0;
    currentSum = 0;
    numCurrentSamples = 0;

    // Setting piezo to zero
    if (enableSerial) {
        setPiezo(piezo.chX_P, 0);
        setPiezo(piezo.chX_N, 0);
        setPiezo(piezo.chY_P, 0);
        setPiezo(piezo.chY_N, 0);
    }

    // Setting sample piezo
    setPiezo(piezo.samplePad, 37500); // pad to about -0.5V (empirical)


    delay(1);
}

void ScanHead::calibrateZeroCurrent() {
    // clearing initial current readout
    current = fetchCurrent();

    delay(500);

    calibratedNoCurrent = fetchCurrent();
    Serial.print("Calibrated zero-current to ");
    Serial.print(calibratedNoCurrent);
    Serial.println("pA");
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

    //Serial.println("IN SETPOSITIONSTEP");

    status = 0;

    current = fetchCurrent();

    float  xerr = (float) xpos_set-xpos;
    float  yerr = (float) ypos_set-ypos;
    float  zerr = (float) zcurr_set-currentRaw;

    float xDerErr = xerr-xPrevErr;
    float yDerErr = yerr-yPrevErr;
    float zDerErr = zerr-zPrevErr;

    xIntErr += xerr;
    yIntErr += yerr;
    zIntErr += zerr;

    xPrevErr = xerr;
    yPrevErr = yerr;
    zPrevErr = zerr;

    int xStepIncrement = (int) (xerr*pidTransverseP + xIntErr*pidTransverseI + xDerErr*pidTransverseD);
    int yStepIncrement = (int) (yerr*pidTransverseP + yIntErr*pidTransverseI + yDerErr*pidTransverseD);
    int zStepIncrement = (int) (zerr*pidZP + zIntErr*pidZI + zDerErr*pidZD);

    //Serial.print("  zerr:");
    //Serial.println(zerr);
    //Serial.print("  zIntErr");
    //Serial.println(zIntErr);
    //Serial.print("  zDerErr");
    //Serial.println(zDerErr);
    //Serial.print(" xi:");
    //Serial.print(xStepIncrement);
    //Serial.print(" zi:");
    //Serial.println(zStepIncrement);

    if (zcurr_set == -1) zStepIncrement = 0;
    else if (zcurr_set == -2) zStepIncrement = -1 * maxZStep;


    //Serial.print("xpos ");
    //Serial.print(xpos);
    //Serial.print(" xpos set ");
    //Serial.println(xpos_set);

    //Serial.print("ypos ");
    //Serial.print(ypos);
    //Serial.print(" ypos set ");
    //Serial.println(ypos_set);


    // checking for overcurrent. If overcurrent, retract and return
    // TODO: uncomment
    //if (current > overCurrent) {
    //    status = 3;
    //    moveStepper(-50, 4096);
    //    return -2;
    //}

    // checking step size, setting step to max size if necessary

    if (abs(xStepIncrement) > maxTransverseStep) xStepIncrement = maxTransverseStep * (xStepIncrement/abs(xStepIncrement));
    if (abs(yStepIncrement) > maxTransverseStep) yStepIncrement = maxTransverseStep * (yStepIncrement/abs(yStepIncrement));
    if (abs(zStepIncrement) > maxZStep) zStepIncrement = maxZStep * (zStepIncrement/abs(zStepIncrement));

    //Serial.print("xStepIncrement ");
    //Serial.println(xStepIncrement);
    //Serial.print("yStepIncrement ");
    //Serial.println(xStepIncrement);

    //Serial.print("zcurr_set ");
    //Serial.print(zcurr_set);
    //Serial.print(" current ");
    //Serial.print(current);
    //Serial.print(" zStepIncrement ");
    //Serial.println(zStepIncrement);

    // applying step

    xpos += xStepIncrement;
    ypos += yStepIncrement;
    zpos += zStepIncrement;

    //Serial.println(zpos);

    //Serial.print("new xpos ");
    //Serial.println(xpos);

    //Serial.print("new ypos ");
    //Serial.println(ypos);

    //Serial.print("new zpos ");
    //Serial.println(zpos);


    int chX_P = maxPiezo/2 - zpos + xpos;
    int chX_N = maxPiezo/2 - zpos - xpos;
    int chY_P = maxPiezo/2 - zpos + ypos;
    int chY_N = maxPiezo/2 - zpos - ypos;

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

    delay(1);

    if (exceeded_bounds == true) {
        Serial.println("exceeded bounds!");
        return -1;
    }
    else if (xpos == xpos_set && ypos == ypos_set) {
        //Serial.println("Reached target, returning");
        return 1;
    }
    else {
        //Serial.println("Have not reached target, returning");
        return 0;
    }

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

int ScanHead::autoApproachStep(int zcurr_set, CircularBuffer<int,1000> &currentBuf, CircularBuffer<int,1000> &zposBuf) {
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
        currentBuf.push(currentRaw); //TODO: exchange with not-raw
        zposBuf.push(zpos);
        //if (approachStatus != 0 and approachStatus != 1) {
        //    Serial.print("could not approach, returned status ");
        //    Serial.println(approachStatus);
        //    break;
        //}
        if (current > zcurr_set) {
           return 1;
        }
    }

    return 0;
}

void ScanHead::sampleCurrent(CircularBuffer<int,50000> &buf) {
    /*!
     * \brief takes a single current sample for integration
     */

    if (enableSerial) {
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

        buf.push(receivedVal); // TODO: remove this

        currentSum += (int) tiafilter.filter( (float) receivedVal);
        currentSumRaw += receivedVal;
        currentLogSum += receivedVal; // TODO: change to something with 60Hz filtering
    }

    numCurrentSamples += 1;
    numCurrentLogSamples += 1;
}

int ScanHead::fetchCurrent() {
    /*!
     * \brief calculates current from integration, clears current integration
     * \detail to provide maximum integration time, call this as infrequently as possible
     * @return current in pA
     */


    current = tiaToCurrent(currentSum / numCurrentSamples);
    currentRaw = tiaToCurrent(currentSumRaw / numCurrentSamples);
    //Serial.print("raw current ");
    //Serial.println(tiaToCurrent(currentSumRaw / numCurrentSamples));

    //Serial.print("Num samples ");
    //Serial.println(numCurrentSamples);
    //Serial.print("Calc current ");
    //Serial.println(current);

    currentSum = 0;
    currentSumRaw = 0;
    numCurrentSamples = 0;

    return current; // might bias results to lower val due to rounding err, but we're ok with this

}

int ScanHead::fetchCurrentLog() {
    /*!
     * \brief calculates current from integration since last fetchCurrentLog call, returns value
     * \detail to provide maximum integration time, call this as infrequently as possible
     * @return current in pA
     */

    int currentLog = tiaToCurrent(currentLogSum / numCurrentLogSamples);

    currentLogSum = 0;
    numCurrentLogSamples = 0;

    return currentLog; // might bias results to lower val due to rounding err, but we're ok with this

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

    return (int) ((float)currentTIA) * 3.3 / 65536.0 * 10000.0 - calibratedNoCurrent;
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

int ScanHead::scanOneAxis(int *currentArr, int *zposArr, int size, int step, bool direction, bool heightControl) {
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

    Serial.println("scanning x-axis");
    Serial.print("Start:");
    Serial.println(xStarting);
    Serial.print("End:");
    Serial.println(xEnding);
    Serial.print("Step:");
    Serial.println(step);

    int currentBuf[size];

    elapsedMicros testStart;

    //while (xpos != xEnding) {
    while(numSteps < size) {


        int setPositionStatus = 0;
        while (setPositionStatus == 0) {
            //Serial.print("Setting position to:");
            //Serial.println(xTarget);
            setPositionStatus = setPositionStep(xTarget, ypos, setCurrent);
            //Serial.println(setPositionStatus);
        }
        //while (setPositionStatus == 0) setPositionStatus = setPositionStep(xStarting, ypos, setCurrent); TODO: ???
        //currentBuf[numSteps] = current;

        //Serial.print("status:");
        //Serial.println(setPositionStatus);

        //Serial.print("zpos:");
        //Serial.println(zpos);

        if (numSteps % step == 0) {
            Serial.print("@");
            Serial.println(xTarget);
            currentArr[numSteps/step] = fetchCurrentLog();
            zposArr[numSteps/step] = zpos;
        }

       if (setPositionStatus != 1) {
        //    for (int i = 0; i < numSteps; i++) {
        //        Serial.println(currentBuf[i]);
        //    }
            Serial.println("Scan failed with error");
            Serial.println(setPositionStatus);
            return setPositionStatus;
        }

        if (direction) xTarget += 1;
        else xTarget -= 1;
        numSteps += 1;
    }

    //Serial.print("time");
    //Serial.println(testStart);

    //for (int i = 0; i < numSteps; i++) {
    //    Serial.println(currentBuf[i]);
    //}

    return 0;

}

int ScanHead::scanTwoAxes(int *currentArr, int *zposArr, int *xposArr, int *yposArr, int sizeX, int sizeY, int step, bool heightControl) {
    /*!
     * \brief two dimensional scan across sizeX and sizeY. Scans over X preferentially. Writes z positions and currents to arrays
     * @param *currentArr: Pointer to sizeX*sizeY long array to store currents
     * @param *zposArr: Pointer to sizeX*sizeY long array to store z positions
     * @param sizeX number of piezo LSBs to scan over in X
     * @param sizeY number of piezo LSBs to scan over in y
     * @param heightControl true if height control enabled, false otherwise
     */

    int xStart = xpos;
    int yStart = ypos;

    int xEnd = xStart + sizeX;
    int yEnd = yStart + sizeY;

    int numStepsExpected = (sizeX * sizeY)/(step*step);

    int setCurrent = setpoint;
    if (!heightControl) setCurrent = -1; // no height control if -1 passed to setPositionStep

    int numSteps = 0;

    Serial.println("scanning x-axis,y-axis");
    Serial.print("Start:");
    Serial.print(xStart);
    Serial.print(",");
    Serial.println(yStart);
    Serial.print("End:");
    Serial.print(xEnd);
    Serial.print(",");
    Serial.println(yEnd);

    bool direction = true;

    for (int yTarget = yStart; yTarget < yEnd; yTarget += step) {

        if (direction) {

            for (int xTarget = xStart; xTarget < xEnd; xTarget += step) {
                Serial.print(xTarget);
                Serial.print("|");
                Serial.println(yTarget);
                int setPositionStatus = 0;
                while (setPositionStatus == 0) setPositionStatus = setPositionStep(xTarget, yTarget, setCurrent);

                if (setPositionStatus != 1) {
                    Serial.println("Scan failed with error");
                    Serial.println(setPositionStatus);
                    return setPositionStatus;
                }

                currentArr[numSteps] = fetchCurrentLog();
                zposArr[numSteps] = zpos;
                xposArr[numSteps] = xpos;
                yposArr[numSteps] = ypos;

                numSteps += 1;
            }
        }

        else {

            for (int xTarget = xEnd; xTarget > xStart; xTarget -= step) {
                Serial.print(xTarget);
                Serial.print("|");
                Serial.println(yTarget);
                int setPositionStatus = 0;
                while (setPositionStatus == 0) setPositionStatus = setPositionStep(xTarget, yTarget, setCurrent);

                if (setPositionStatus != 1) {
                    Serial.println("Scan failed with error");
                    Serial.println(setPositionStatus);
                    return setPositionStatus;
                }

                currentArr[numSteps] = fetchCurrentLog();
                zposArr[numSteps] = zpos;
                xposArr[numSteps] = xpos;
                yposArr[numSteps] = ypos;

                numSteps += 1;
            }
        }

        direction = !direction;


        //while (setPositionStatus == 0) setPositionStatus = setPositionStep(xStarting, ypos, setCurrent); TODO: ???
        //currentBuf[numSteps] = current;

        //Serial.print("status:");
        //Serial.println(setPositionStatus);

        //Serial.print("zpos:");
        //Serial.println(zpos);

    }

    //Serial.print("time");
    //Serial.println(testStart);

    //for (int i = 0; i < numSteps; i++) {
    //    Serial.println(currentBuf[i]);
    //}

    return 0;

}
