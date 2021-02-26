#include <Arduino.h>
#include <CircularBuffer.h>
#include "scanhead.h"
#include "ui.h"

ScanHead *scanhead;
UI *ui;

int setpoint = 500; // 500pA

IntervalTimer currentSampleTimer;

void approachLoop(CircularBuffer<int,1000> &current, CircularBuffer<int,1000> &zpos, bool userStart) {
    /*!
     * brief: provides manual and automatic control over scan head during stepper aproach
     * @param *zpos: a circular buffer which will hold z positions during approach
     * @param *current: a circular buffer which will hold currents during approach
     */


    if (userStart) {
        while (ui->encoderVals.next == 1) {
            ui->updateInputs();
        }


        // move encoder until user selection
        while (ui->encoderVals.next == 0) {
            ui->updateInputs();
            scanhead->fetchCurrent();
            scanhead->moveStepper(1, ui->encoderVals.encoderPos);
            current.push(scanhead->current);
            zpos.push(scanhead->zpos);
            ui->drawDisplay(scanhead);
            //Serial.println(scanhead->fetchCurrent());
        }
    }

    // auto approach
    int autoApproachStatus = 0;
    int autoApproachSteps = 0;

    while (autoApproachStatus == 0) {
        Serial.print("approach step, currentRaw=");
        Serial.println(scanhead->currentRaw);
        autoApproachStatus = scanhead->autoApproachStep(setpoint, current, zpos);
        autoApproachSteps += 1;
        if (autoApproachSteps % 1 == 0) {
            ui->updateInputs();
            ui->drawDisplay(scanhead);
        }
    }

    for (int i = 0; i < 1000; i++) scanhead -> setPositionStep(0,0,setpoint);
}

void sampleScanHeadCurrent() {
    scanhead->sampleCurrent();
}

void setup() {
    // Initial Setup
    Serial.begin(9600);
    delay(1000); // todo: remove
    Serial.println("OpenSTM V0.1 Startup...");

    Serial.println("Initializing ScanHead");

    scanhead = new ScanHead();
    // setting up interrupt for current integration
    currentSampleTimer.begin(sampleScanHeadCurrent, 50); // sampling every 50us -> 20kHz

    Serial.println("Initializing UI");

    ui = new UI();

    Serial.println("Calibrating Zero Current");
    scanhead->calibrateZeroCurrent();

    Serial.println("Startup Complete");

    ui->drawDisplay(scanhead);
    ui->updateInputs();

    // Initial Approach

    CircularBuffer<int,1000> currentBuffer;
    CircularBuffer<int,1000> zPosBuffer;

    Serial.println("Starting Approach");
    approachLoop(currentBuffer, zPosBuffer, true);
    Serial.print("found current ");
    Serial.println(scanhead->current);
    Serial.println("Approach complete");

    //// 1-D Scan, without height control


    int numpoints = 500;
    int step = 1;

    int currents1Df[numpoints/step];
    int zPos1Df[numpoints/step];
    int currents1Db[numpoints/step];
    int zPos1Db[numpoints/step];

    int scanStatus;


    Serial.println("Scanning +x");

    for (int i = 0; i < 1; i++) {

        scanStatus = scanhead->scanOneAxis(currents1Df, zPos1Df, numpoints, step, true, true); // scanning on +x


        ui -> drawDisplay(scanhead);

        if (scanStatus != 0) {
            Serial.print("Encountered scan error ");
            Serial.println(scanStatus);
        }

        //approachLoop(currentBuffer, zPosBuffer, false); // returning to setpoint

        Serial.println("Scanning -x");

        scanStatus = scanhead->scanOneAxis(currents1Db, zPos1Db, numpoints, step, false, true); // scanning on -x

        ui -> drawDisplay(scanhead);

        if (scanStatus != 0) {
            Serial.print("Encountered scan error ");
            Serial.println(scanStatus);
        }

        Serial.println("Dumping approach data...");


        for (int j = 0; j < 1000; j++) {
            Serial.print(currentBuffer.shift());
            Serial.print(",");
            Serial.println(zPosBuffer.shift());
        }

        Serial.println("Dumping forward x-axis scan");

        //delay(5000);

        for (int j = 0; j < numpoints/step; j++) {
                Serial.print(j);
                Serial.print(",");
                Serial.print(currents1Df[j]);
                Serial.print(",");
                Serial.println(zPos1Df[j]);
        }

        Serial.println("Dumping backward x-axis scan");

        for (int j = 0; j < numpoints/step; j++) {
                Serial.print(j);
                Serial.print(",");
                Serial.print(currents1Db[j]);
                Serial.print(",");
                Serial.println(zPos1Db[j]);
        }

        for (int step = 0; step < 50; step++) scanhead->moveStepper(1, -10);
    }

    //approachLoop(currentBuffer, zPosBuffer); // returning to setpoint

}


void loop() {
    //scanhead->testScanHeadPosition(30000,10);
}
