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
        int scanOneAxis(int *currents, int *zpos, int size, bool direction, bool heightcontrol);

    private:

        int currentToTia(int currentpA);
        int tiaToCurrent(int currentTIA);
        void setPiezo(int channel, int value);

        int setpoint; // current setpoint

        Stepper stepper0;
        Stepper stepper1;
        Stepper stepper2;

        struct piezo_struct {
            static const int cs = 10;
            static const int chX_P = 1;
            static const int chY_P = 3;
            static const int chY_N = 5;
            static const int chX_N = 7;
        } piezo;

        struct tia_struct {
            static const int cs   = 34;
            static const int din  = 38;
            static const int miso = 39;
        } tia;
        struct stepper0_struct {
            static const int A = 5;
            static const int B = 4;
            static const int C = 3;
            static const int D = 2;
        } stepper0_pins;
        struct stepper1_struct {
            static const int A = 9;
            static const int B = 8;
            static const int C = 7;
            static const int D = 6;
        } stepper1_pins;
        struct stepper2_struct {
            static const int A = 32;
            static const int B = 33;
            static const int C = 25;
            static const int D = 24;
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
