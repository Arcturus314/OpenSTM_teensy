/*
 * ui.h
 * Kaveh Pezeshki 1/24/2021
 * Models UI interaction with STM control board
 */

#ifdef ui_h
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
        void updateInputs();

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

        struct voltageVals_struct = {
            int _5        = 0;
            int _10       = 0;
            int _33       = 0;
            bool _5_good  = true;
            bool _10_good = true;
            bool _33_good = true;
        } voltageVals;

    private:

        void plotBarsLog(int current);

        Encoder enc;
        Adafruit_24bargraph bar;
        Adafruit_SSD1306 display;


        struct dpad_struct {
            const int l = 15;
            const int r = 14;
            const int u = 36;
            const int d = 35;
            const int c = 40;
        } dpad;

        struct joystick_struct {
            const int xax = A9;
            const int yax = A8;
        } joystick;

        struct encoder_struct {
            const int chA  = 30;
            const int chB  = 31;
            const int next = 37;
            const int sel  = 41;
        } encoder;

        struct voltages_struct {
            const int _5 = A7;
            const int _10 = A6;
            const int _33 = A5;
            const int allowedvariance = 20;
            const int _5_expected = 488; // calibrated per-board
            const int _10_expected = 573; // calibrated per-board
            const int _33_expected = 492; // calibrated per-board
        } voltages;

        struct display_struct {
            const int width  = 128;
            const int height = 64;
        } display;

        struct debug_struct {
            const int io3 = 18;
        } debug;

        



#endif
