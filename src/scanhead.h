/*
 * scanhead.h
 * Kaveh Pezeshki 1/18/2021
 * Tracks and controls position of STM scan tip
 */

#ifndef scanhead_h
#define scanhead_h

#include "Arduino.h"
#include "Stepper.h"

class ScanHead
{
    public:
        ScanHead();

        int current;

        // Current status of the scan head
        // 0: approach step
        // 1: approach piezo
        // 2: scan
        // 3: overcurrent
        int status;

        int xpos;
        int ypos;
        int zpos;
        int zposStepper;
        int setPositionStep(int xpos_set, int ypos_set, int zcurr_set);
        void moveStepper(int steps, int stepRate);
        int autoApproachStep(int zcurr_set);
        int fetchCurrent();

    private:

        int currentToTia(int currentpA);
        int tiaToCurrent(int currentTIA);
        void setPiezo(int channel, int value);

        Stepper stepper0;
        Stepper stepper1;
        Stepper stepper2;

        struct piezo_struct {
            const int cs = 10;
            const int chX_P = 1;
            const int chY_P = 3;
            const int chY_N = 5;
            const int chX_N = 7;
        } piezo;

        struct tia_struct {
            const int cs   = 34;
            const int din  = 38;
            const int miso = 39;
        } tia;
        struct stepper0_struct {
            const int A = 5;
            const int B = 4;
            const int C = 3;
            const int D = 2;
        } stepper0_pins;
        struct stepper1_struct {
            const int A = 9;
            const int B = 8;
            const int C = 7;
            const int D = 6;
        } stepper1_pins;
        struct stepper2_struct {
            const int A = 32;
            const int B = 33;
            const int C = 25;
            const int D = 24;
        } stepper2_pins;

        const float calibratedNoCurrent = 5625.0; // no-current TIA reading, empirical
        const int   pidTransverseP = 1; // gain term in PID control for transverse axes
        const int   pidZP = 1; // gain term in PID control for Z axis
        const int   maxPiezo = 65535; // maximum valuable attainable by a single piezo channel
        const int   minPiezo = 0; // minimum valuable attainable by a single piezo channel

        const int maxTransverseStep = 100; // largest one-cycle piezo step on the x-axis
        const int maxZStep = 100;

        const int currentSet = 300;    // 0.3nA
        const int overCurrent = 20000; // 20nA, corresponding to 2/3 of TIA FSD

};

#endif
