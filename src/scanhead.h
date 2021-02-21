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

        static int current;

        // Current status of the scan head
        // 0: approach step
        // 1: approach piezo
        // 2: scan
        // 3: overcurrent
        static int status;

        static int xpos;
        static int ypos;
        static int zpos;
        static int zposStepper;
        static int setPositionStep(int xpos_set, int ypos_set, int zcurr_set);
        static void moveStepper(int steps, int stepRate);
        static int autoApproachStep(int zcurr_set);
        static int fetchCurrent();
        static int scanOneAxis(int *currents, int *zpos, int size, bool direction, bool heightcontrol);
        static void testScanHeadPosition(int numsteps, int stepsize);

        static struct stepper0_struct {
            static const int A = 5;
            static const int B = 4;
            static const int C = 3;
            static const int D = 2;
        } stepper0_pins;
        static struct stepper1_struct {
            static const int A = 9;
            static const int B = 8;
            static const int C = 7;
            static const int D = 6;
        } stepper1_pins;
        static struct stepper2_struct {
            static const int A = 32;
            static const int B = 33;
            static const int C = 25;
            static const int D = 24;
        } stepper2_pins;

    private:

        static int currentToTia(int currentpA);
        static int tiaToCurrent(int currentTIA);
        static void sampleCurrent();

        static void setPiezo(int channel, int value);

        static int setpoint; // current setpoint

        static int currentSum;
        static int numCurrentSamples;

        static IntervalTimer currentSampleTimer;

        static Stepper stepper0;
        static Stepper stepper1;
        static Stepper stepper2;

        static struct piezo_struct {
            static const int cs = 10;
            static const int chX_P = 1;
            static const int chY_P = 3;
            static const int chY_N = 5;
            static const int chX_N = 7;
            static const int samplePad = 0;
        } piezo;

        static struct tia_struct {
            static const int cs   = 34;
            static const int din  = 38;
            static const int miso = 39;
        } tia;

        static constexpr float calibratedNoCurrent = 3532.6; // no-current TIA reading, empirical. Note - stdev of 49, 750 samples
        static const int   pidTransverseP = 1; // gain term in PID control for transverse axes
        static const int   pidZP = 1; // gain term in PID control for Z axis
        static const int   maxPiezo = 65535; // maximum valuable attainable by a single piezo channel
        static const int   minPiezo = 0; // minimum valuable attainable by a single piezo channel

        static const int maxTransverseStep = 100; // largest one-cycle piezo step on the x-axis
        static const int maxZStep = 100;

        static const int currentSet = 300;    // 0.3nA
        static const int overCurrent = 20000; // 20nA, corresponding to 2/3 of TIA FSD

};

#endif
