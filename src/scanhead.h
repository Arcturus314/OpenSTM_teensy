/*
 * scanhead.h
 * Kaveh Pezeshki 1/18/2021
 * Tracks and controls position of STM scan tip
 */

#ifndef scanhead_h
#define scanhead_h

#include "Arduino.h"
#include "Stepper.h"
#include "SPI.h"
#include <CircularBuffer.h>
#include "biquad.cpp"

class ScanHead
{
    public:
        ScanHead();

        int current;
        int currentRaw;

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
        int autoApproachStep(int zcurr_set, CircularBuffer<int,1000> &currentBuf, CircularBuffer<int,1000> &zposBuf);
        int fetchCurrent();
        int fetchCurrentLog();
        void calibrateZeroCurrent();
        void sampleCurrent();
        int scanTwoAxes(int *currentArr, int *zposArr, int *xposArr, int *yposArr, int sizeX, int sizeY, bool heightcontrol);
        int scanOneAxis(int *currentArr, int *zposArr, int size, int step, bool direction, bool heightcontrol);
        void testScanHeadPosition(int numsteps, int stepsize);

    private:

        const bool enableSerial = true; // serial enable for non-setup serial (SPI) ops

        int currentToTia(int currentpA);
        int tiaToCurrent(int currentTIA);

        void setPiezo(int channel, int value);

        int setpoint; // current setpoint

        int currentSum;
        int currentSumRaw;
        int numCurrentSamples;

        int currentLogSum;
        int numCurrentLogSamples;

        float calibratedNoCurrent = 0; // This is re-measured when the STM boots.

        Biquad tiafilter;

        Stepper stepper0;
        Stepper stepper1;
        Stepper stepper2;

        struct piezo_struct {
            static const int cs = 10;
            static const int chX_P = 1;
            static const int chY_P = 3;
            static const int chY_N = 5;
            static const int chX_N = 7;
            static const int samplePad = 0;
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

        const int   maxPiezo = 65535; // maximum valuable attainable by a single piezo channel
        const int   minPiezo = 0; // minimum valuable attainable by a single piezo channel

        const float pidTransverseP = 1; // gain term in PID control for transverse axes
        const float pidZP = 0.5; // gain term in PID control for Z axis
        const float pidTransverseI = 0.0; // Integral term in PID control for transverse axes
        const float pidZI = 0.0;  // Integral term in PID control for Z axis
        const float pidTransverseD = 0.0; // Derivative term in PID control for transverse axes
        const float pidZD = 0.0; // Derivative term in PID control for Z axis


        const int maxTransverseStep = 100; // largest one-cycle piezo step on the x-axis
        const int maxZStep = 100;

        // PID internal use

        float xIntErr = 0;
        float yIntErr = 0;
        float zIntErr = 0;

        float xPrevErr = 0;
        float yPrevErr = 0;
        float zPrevErr = 0;

        const int currentSet = 300;    // 0.3nA
        const int overCurrent = 20000; // 20nA, corresponding to 2/3 of TIA FSD

        // const float calibratedNoCurrent = 3532.6; // no-current TIA reading, empirical. Note - stdev of 49, 750 samples

        // Coefficients for 60Hz band-stop filter implemented with a second order Butterworth filter. Coefficients
        // generated through python script:
        //
        //# with 20khz sample rate, 1 is 10khz
        //# we want to cut out 59hz to 61hz
        //wp = [0.0058, 0.0062]
        //ws = [0.0059, 0.0061]
        //gpass = 1
        //gstop = 20
        //systemsos = signal.iirdesign(wp, ws, gpass, gstop, output='sos')
        //print(systemsos)

        const double tia60hzbiquad[6] = {0.99884837,-1.99734223,0.99884837,1,-1.99772833,0.99808291};

};

#endif
