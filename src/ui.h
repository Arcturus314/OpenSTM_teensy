/*
 * ui.h
 * Kaveh Pezeshki 1/24/2021
 * Models UI interaction with STM control board
 */

#ifndef ui_h
#define ui_h

#include "scanhead.h"
#include "Arduino.h"
#include "Encoder.h"
#include "Wire.h"
#include "Adafruit_GFX.h"
#include "Adafruit_LEDBackpack.h"
#include "Adafruit_SSD1306.h"

class UI
{
    public:
        UI();
        void drawDisplay(ScanHead* scanhead);
        void drawDisplayErr(int error);
        void updateInputs();

        struct barPlot_struct {
            int minBound = 50; // pA. This is the minimum value that will be plotted
            int maxBound = 20000; // 20 nA, corresponding to 2/3 of TIA FSD. This is the maximum value that will be plotted
            int lowBound = 300; // pA. This is the lower bound of the useful current range
            int highBound = 3000; // pA. This is the upper bound of the useful current range
        } barPlot;

        struct dpadVals_struct {
            int l = 0;
            int r = 0;
            int u = 0;
            int d = 0;
            int c = 0;
        } dpadVals;

        struct joystickVals_struct {
            int xax = 0;
            int yax = 0;
        } joystickVals;

        struct encoderVals_struct {
            int encoderPos = 0;
            int next = 0;
            int sel = 0;
        } encoderVals;

        struct voltageVals_struct {
            int _5        = 0;
            int _10       = 0;
            int _33       = 0;
            bool _5_good  = true;
            bool _10_good = true;
            bool _33_good = true;
        } voltageVals;

    private:

        Encoder enc;
        Adafruit_24bargraph bar;
        Adafruit_SSD1306 display;

        void plotBarsLog(int current);

        struct dpad_struct {
            static const int l = 15;
            static const int r = 14;
            static const int u = 36;
            static const int d = 35;
            static const int c = 40;
        } dpad;

        struct joystick_struct {
            static const int xax = A9;
            static const int yax = A8;
        } joystick;

        struct encoder_struct {
            static const int chA  = 30;
            static const int chB  = 31;
            static const int next = 37;
            static const int sel  = 41;
        } encoder;

        struct voltages_struct {
            static const int _5 = A7;
            static const int _10 = A6;
            static const int _33 = A5;
            static const int allowedvariance = 20;
            static const int _5_expected = 488; // calibrated per-board
            static const int _10_expected = 573; // calibrated per-board
            static const int _33_expected = 492; // calibrated per-board
        } voltages;

        struct display_config_struct {
            static const int width  = 128;
            static const int height = 64;
        } display_config;

        struct debug_struct {
            static const int io3 = 18;
        } debug;
};

#endif
