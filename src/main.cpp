#include <Arduino.h>
#include <CircularBuffer.h>
#include "scanhead.h"
#include "ui.h"

ScanHead *scanhead;
UI *ui;

int setpoint = 500; // 500pA

void approachLoop(CircularBuffer<int,1000> &current, CircularBuffer<int,1000> &zpos) {
    /*!
     * brief: provides manual and automatic control over scan head during stepper aproach
     * @param *zpos: a circular buffer which will hold z positions during approach
     * @param *current: a circular buffer which will hold currents during approach
     */

    // move encoder until user selection
    while (ui->encoderVals.next == 0) {
        ui->updateInputs();
        scanhead->moveStepper(1, ui->encoderVals.encoderPos*100);
        current.push(scanhead->current);
        zpos.push(scanhead->zpos);
        ui->drawDisplay(scanhead);
    }

    // auto approach
    int autoApproachStatus = 1;
    int autoApproachSteps = 0;

    while (autoApproachStatus == 1) {
        autoApproachStatus = scanhead->autoApproachStep(setpoint);
        autoApproachSteps += 1;
        current.push(scanhead->current);
        zpos.push(scanhead->zpos);
        if (autoApproachSteps % 100 == 0) {
            ui->drawDisplay(scanhead);
        }
    }
}

void setup() {
    // Initial Setup
    Serial.begin(9600);
    Serial.println("OpenSTM V0.1 Startup...");

    scanhead = new ScanHead();
    ui = new UI();

    Serial.println("Startup Complete");

    // Initial Approach

    CircularBuffer<int,1000> currentBuffer;
    CircularBuffer<int,1000> zPosBuffer;

    Serial.println("Starting Approach");
    approachLoop(currentBuffer, zPosBuffer);
    Serial.println("Approach Complete. Dumping approach data...");

    for (int i = 0; i < 1000; i++) {
        Serial.print(currentBuffer.first());
        Serial.print(",");
        Serial.println(zPosBuffer.first());
    }


    // 1-D Scan, without height control
    
    int currents1D[1000];
    int zPos1D[1000];

    int scanStatus;

    Serial.println("Scanning +x");

    scanStatus = scanhead->scanOneAxis(*currents1D, *zPos1D, 1000, true, false); // scanning on +x

    if (scanStatus != 0) {
        Serial.print("Encountered scan error ");
        Serial.println(scanStatus);
    }

    Serial.println("Dumping forward x-axis scan");

    for (int i = 0; i < 1000; i++) {
        Serial.print(currents1D[i]);
        Serial.print(",");
        Serial.println(zPos1D[i]);
    }

    approachLoop(currentBuffer, zPosBuffer); // returning to setpoint

    Serial.println("Scanning +x");

    scanStatus = scanhead->scanOneAxis(*currents1D, *zPos1D, 1000, false, false); // scanning on -x

    if (scanStatus != 0) {
        Serial.print("Encountered scan error ");
        Serial.println(scanStatus);
    }

    Serial.println("Dumping forward x-axis scan");

    for (int i = 0; i < 1000; i++) {
        Serial.print(currents1D[i]);
        Serial.print(",");
        Serial.println(zPos1D[i]);
    }

    approachLoop(currentBuffer, zPosBuffer); // returning to setpoint

}


void loop() {
    Serial.println("scan complete...");
    delay(10000);
}
