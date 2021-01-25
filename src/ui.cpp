/*
 * ui.cpp
 * Kaveh Pezeshki 1/24/2021
 * Models UI interaction with STM control board
 */

#include "Arduino.h"
#include "ui.h"

UI::UI():
    enc(encoder.chA, encoder.chB),
    display(display.width, display.height, &Wire1, -1)
    bar()
{
    pinMode(encoder.sel, INPUT);
    pinMode(encoder.next, INPUT);
    pinMode(dpad.l, INPUT);
    pinMode(dpad.r, INPUT);
    pinMode(dpad.u, INPUT);
    pinMode(dpad.d, INPUT);
    pinMode(dpad.c, INPUT);

    bar.begin(0x70);
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    // fun bar graphics
    for (int i = 0; i < 25; i++) {
    for (int b = 0; b < i; b++) {
      bar.setBar(24 - b, LED_GREEN);
    }
    for (int b = i; b < 25; b++) {
      bar.setBar(24 - b, LED_OFF);
    }
    bar.writeDisplay();
    delay(100);
    }

    delay(100);
    for (int b = 0; b < 24; b++) {
    bar.setBar(b, LED_OFF);
    }

    bar.writeDisplay();

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("UI init complete");
    display.display();

    delay(500);
}

void UI::updateInputs() {
   // updating all inputs

   // dpad
   dpadVals.l = digitalRead(dpad.l);
   dpadVals.r = digitalRead(dpad.r);
   dpadVals.u = digitalRead(dpad.u);
   dpadVals.d = digitalRead(dpad.d);
   dpadVals.c = digitalRead(dpad.c);

   // joystick
   joystickVals.xax = analogRead(joystick.xax);
   joystickVals.yax = analogRead(joystick.yax);

   // encoder
   encoderVals.encoderPos = enc.read();
   encoderVals.next = digitalRead(encoder.next);
   encoderVals.sel = digitalRead(encoder.sel);

   // voltages
   voltageVals._5 = analogRead(voltages._5);
   voltageVals._10 = analogRead(voltages._10);
   voltageVals._33 = analogRead(voltages._33);

   if (voltageVals._5 < voltages._5_expected - 20 || voltageVals._5 > voltages._5_expected + 20 )
       voltages.5_good = false;
   else voltages.5_good = true;

   if (voltageVals._10 < voltages._10_expected - 20 || voltageVals._10 > voltages._10_expected + 20 )
       voltages.10_good = false;
   else voltages.10_good = true;

   if (voltageVals._33 < voltages._33_expected - 20 || voltageVals._33 > voltages._33_expected + 20 )
       voltages.33_good = false;
   else voltages.33_good = true;
}
