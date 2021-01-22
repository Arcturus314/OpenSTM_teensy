/*
 * scanhead.h
 * Kaveh Pezeshki 1/18/2021
 * Tracks and controls position of STM scan tip
 */

#ifndef scanhead_h
#define scanhead_h

#include "Arduino.h"

class ScanHead
{
    public:
        ScanHead();


        int xpos;
        int ypos;
        int zpos;
        int zposStepper;
        int setPositionStep(int xpos, int ypos, int zpos);
        void moveStepper(int steps, int stepRate);
        int autoApproachStep();
        int fetchCurrent();
    private:


        struct piezo_struct {
            const int cs = 10;
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


        void moveSingleStepper(int stepper);

};

#endif
