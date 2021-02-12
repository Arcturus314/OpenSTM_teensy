/*
 * ui.cpp
 * Kaveh Pezeshki 1/24/2021
 * Models UI interaction with STM control board
 */

#include "ui.h"
#include "Arduino.h"
#include <math.h>

UI::UI():
    enc(encoder.chA, encoder.chB),
    display(display_config.width, display_config.height, &Wire1, -1),
    //display(128, 64, &Wire1, -1),
    bar()
{

    Serial.println("Setting up UI");

    pinMode(encoder.sel, INPUT);
    pinMode(encoder.next, INPUT);
    pinMode(dpad.l, INPUT);
    pinMode(dpad.r, INPUT);
    pinMode(dpad.u, INPUT);
    pinMode(dpad.d, INPUT);
    pinMode(dpad.c, INPUT);

    Serial.println("Setting up Bar");

    bar.begin(0x70);

    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
      Serial.println("SSD1306 init failed");
    }

    Serial.println("Display on bar");

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

    Serial.println("UI init complete");

    delay(500);
}

void UI::updateInputs() {
   /*!
    * \brief Scans all inputs on the control board and updates structs
    */

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
       voltageVals._5_good = false;
   else voltageVals._5_good = true;

   if (voltageVals._10 < voltages._10_expected - 20 || voltageVals._10 > voltages._10_expected + 20 )
       voltageVals._10_good = false;
   else voltageVals._10_good = true;

   if (voltageVals._33 < voltages._33_expected - 20 || voltageVals._33 > voltages._33_expected + 20 )
       voltageVals._33_good = false;
   else voltageVals._33_good = true;
}

void UI::plotBarsLog(int current) {
    /*!
     * \brief Draws new value on the LED bar chart representing the TIA input current
     * @param current TIA current in pA
     */

    // calculating number of bars. Lots of logs that don't need to be calculated for every function call, so room for opt here
    int num_bars = 0;
    if (current > 0) num_bars = (int) (log((float)current)-log((float)barPlot.minBound))/(log((float)barPlot.maxBound)-log((float)barPlot.minBound));

    if (num_bars > 24) num_bars = 24;
    else if (num_bars < 0) num_bars = 0;

    // updating display
    if (current < barPlot.lowBound)         for (int b = 0; b < num_bars; b++) bar.setBar(24 - b, LED_YELLOW);
    else if (current > barPlot.highBound)   for (int b = 0; b < num_bars; b++) bar.setBar(24 - b, LED_RED);
    else for (int b = 0; b < num_bars; b++) bar.setBar(24 - b, LED_GREEN);

    for (int b = num_bars; b < 25; b++) bar.setBar(24 - b, LED_OFF);
    bar.writeDisplay();
}


void UI::drawDisplay(ScanHead* scanhead) {
    /*!
     * \brief Updates values on display, including position and current information from the scanhead
     * @param scanhead ScanHead object to update ScanHead fields
     */
    
    // initial setup
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    // writing scan status
    
    display.setCursor(0,0);
    switch (scanhead->status) {
    case 0:
      display.print("STEPPER AP");
      break;
    case 1:
      display.print("PIEZO AP");
      break;
    case 2:
      display.print("SCAN");
      break;
    case 3:
      display.print("OVERCURR");
      break;
    }

    // writing voltage status: will only display voltages when good    
    display.setCursor(70,0);
    if (voltageVals._5_good) display.print("5V");
    display.setCursor(85,0);
    if (voltageVals._10_good) display.print("10V");
    display.setCursor(100,0);
    if (voltageVals._33_good) display.print("3.3V");

    // writing piezo position
    display.setCursor(0, 20);
    display.println("POS");
    display.setCursor(30, 20);
    display.println(scanhead->xpos / 10);
    display.setCursor(60, 20);
    display.println(scanhead->ypos / 10);
    display.setCursor(90, 20);
    display.println(scanhead->zpos / 10);

    // writing stepper position
    display.setCursor(0, 35);
    display.println("STEPPER");
    display.setCursor(50, 35);
    display.println(scanhead->zposStepper);
    display.setCursor(100, 35);
    display.println(encoderVals.encoderPos);

    // writing current
    display.setCursor(0, 50);
    display.println("CURRENT (pA)");
    display.setCursor(90, 50);
    display.println(scanhead->current);

    display.display();

    plotBarsLog(scanhead->current);
}

void UI::drawDisplayErr(int error) {

    // initial setup
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    // printing error message
    display.print("ERROR ");
    display.setCursor(100,0);
    display.print(error);

    display.display();
}
