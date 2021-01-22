#include "Arduino.h"
#include <SPI.h>
#include <Stepper.h>
#include <Wire.h>
#include <Encoder.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <Adafruit_SSD1306.h>
#include <math.h>
#include "displaydata.h"
#include "position.h"


// Interface Definitions (Device)
// ---PIEZO---
// on SPI0
#define PIEZO_CS 10

// ---TIA---
// on SPI1
#define TIA_CS 34 // Tied to CONVST
#define TIA_DIN 38 // Hold this HIGH
#define TIA_MISO 39

// ---STEPPER1---
#define STEPPER0_A 5
#define STEPPER0_B 4
#define STEPPER0_C 3
#define STEPPER0_D 2

// ---STEPPER2---
#define STEPPER1_A 9
#define STEPPER1_B 8
#define STEPPER1_C 7
#define STEPPER1_D 6

// ---STEPPER3---
#define STEPPER2_A 32
#define STEPPER2_B 33
#define STEPPER2_C 25
#define STEPPER2_D 24

// Interface Definitions (UI)

// ---encoder---
#define ENCA 30
#define ENCB 31
#define SELSW 41
#define NEXTSW 37

// ---d-pad---
#define LSW 15
#define RSW 14
#define USW 36
#define DSW 35
#define CSW 40

// ---joystick---
#define XAX A9
#define YAX A8

// ---voltage---
#define VMON5 A7
#define VMON10 A6
#define VMON33 A5

// ---displays---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// ---debug---
#define DEBUGPIN_IO3 18


// Global variables

// Device
const int stepsPerRevolution = 64; // another 64x mechanical reduction, wrong pinout as half steps
Stepper stepper0(stepsPerRevolution, STEPPER0_A, STEPPER0_C, STEPPER0_B, STEPPER0_D);
Stepper stepper1(stepsPerRevolution, STEPPER1_A, STEPPER1_C, STEPPER1_B, STEPPER1_D);
Stepper stepper2(stepsPerRevolution, STEPPER2_A, STEPPER2_C, STEPPER2_B, STEPPER2_D);

// UI
Encoder enc(ENCA, ENCB);
Adafruit_24bargraph bar = Adafruit_24bargraph();
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);

// Software Settings

// Hardware monitors

#define ALLOWEDRAILVARIANCE 20 // steps, 10-bit
#define VMON5EXPECTED 488 // 10-bit, calibrated to board
#define VMON10EXPECTED 573 // 10-bit, calibrated to board
#define VMON33EXPECTED 492 // 10-bit, calibrated to board


// TIA: note 0.3V bias on V+, 1e8 transimpedance gain, 16 bit ADC @ 3.3V reference

#define DESIREDCURRENT 11915 // 3nA
//#define DESIREDCURRENT 20915 // >3nA
#define OVERCURRENT 45677 // 20nA
#define LOWCURRENT  7943 // 1nA
//#define LOWCURRENT  11943 // >1nA
#define HIGHCURRENT 25817 // 10nA

#define MAXSTEPINCREMENT 10


void setupTIA() {
  pinMode(TIA_CS, OUTPUT);
  pinMode(TIA_DIN, OUTPUT);
  digitalWrite(TIA_DIN, HIGH);
  SPI1.setMISO(TIA_MISO);
  SPI1.begin();
  SPI1.beginTransaction(SPISettings(100000, MSBFIRST, SPI_MODE3));
  delay(10);
}

void setupPiezo() {
  pinMode(PIEZO_CS, OUTPUT);
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE1));
  // turning all channels on
  digitalWrite(PIEZO_CS, LOW);
  SPI.transfer(0b00000100);
  SPI.transfer(0b00000000);
  SPI.transfer(0b00000000);
  SPI.transfer(0b11111111);
  digitalWrite(PIEZO_CS, HIGH);
  // setting int reference
  digitalWrite(PIEZO_CS, LOW);
  SPI.transfer(0b00001000);
  SPI.transfer(0b00000000);
  SPI.transfer(0b00000000);
  SPI.transfer(0b00000001);
  digitalWrite(PIEZO_CS, HIGH);
  delay(10);
}

void setupUI() {

  pinMode(SELSW, INPUT);
  pinMode(NEXTSW, INPUT);
  pinMode(LSW, INPUT);
  pinMode(RSW, INPUT);
  pinMode(USW, INPUT);
  pinMode(DSW, INPUT);
  pinMode(CSW, INPUT);

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

void setupSteppers(int stepSpeed) {
  stepper0.setSpeed(stepSpeed);
  stepper1.setSpeed(stepSpeed);
  stepper2.setSpeed(stepSpeed);
}

void setPiezo(int channel, int value) {
  unsigned int dacMSB = highByte(value);
  unsigned int dacLSB = lowByte(value);

  byte command = 0b00000011;
  byte addrD1  = lowByte(channel) << 4 | dacMSB >> 4;
  byte addrD2  = dacMSB << 4 | dacLSB >> 4;
  byte addrD3  = dacLSB << 4;

  digitalWrite(PIEZO_CS, LOW);
  SPI.transfer(command);
  SPI.transfer(addrD1);
  SPI.transfer(addrD2);
  SPI.transfer(addrD3);
  digitalWrite(PIEZO_CS, HIGH);
}

void setPiezoXYZ(int x, int y, int z) {
  // expects x, y centered around 0 (eg can go to +- 32678)
  // expects z from 0 -> +

  int chX_P = z + x;
  int chX_N = z - x;
  int chY_P = z + y;
  int chY_N = z - y;

  // mapping: +X -> 1, +Y -> 3, -Y -> 5, -X -> 7

  setPiezo(1, chX_P);
  setPiezo(3, chY_P);
  setPiezo(5, chY_N);
  setPiezo(7, chX_N);
}

int readTIA() {
  int sumVal = 0;
  for (int i = 0; i < 5; i++) {
    digitalWrite(TIA_CS, LOW);
    delayMicroseconds(1);
    digitalWrite(TIA_CS, HIGH);
    delayMicroseconds(1);
    digitalWrite(TIA_CS, LOW);
    int16_t receivedVal_high = (int16_t) SPI1.transfer(0xff);
    int16_t receivedVal_low  = (int16_t) SPI1.transfer(0xff);
    delayMicroseconds(1);
    digitalWrite(TIA_CS, HIGH);
    delayMicroseconds(1);
    digitalWrite(TIA_CS, LOW);
    int receivedVal = receivedVal_high << 8 | receivedVal_low;
    sumVal += receivedVal;
  }

  return sumVal / 5; // might bias results to lower val due to rounding err, but we're ok with this
}

void moveStepperSteps(int steps) {
  stepper0.step(steps);
  stepper1.step(steps);
  stepper2.step(steps);
}

void setup() {
  // Overall setup
  Serial.begin(115200);
  Serial.println("Initializing OpenSTM V0.2");
  setupTIA();
  Serial.println("Initialized TIA");
  setupPiezo();
  setPiezo(15, 32768); // resetting all channels to midrange
  Serial.println("Initialized Piezo Driver");
  setupSteppers(100);
  Serial.println("Initialized Steppers");
  setupUI();
  Serial.println("Initialized UI");

  // Basic readouts
  pinMode(DEBUGPIN_IO3, OUTPUT);
}

void drawDisplay(displayData data) {
  // initial setup
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  // writing status codes
  switch (data.status) {
    case 0:
      display.print("NO ERR");
      break;
    case 1:
      display.print("10V ERR");
      break;
    case 2:
      display.print("5V ERR");
      break;
    case 3:
      display.print("3.3V ERR");
      break;
    case 4:
      display.print("APPROACH_STEP");
      break;
    case 5:
      display.print("APPROACH_PIEZO");
      break;
    case 6:
      display.print("SCAN");
      break;
    case 7:
      display.print("RETRACT");
      break;
  }

  // writing piezo position
  display.setCursor(0, 20);
  display.println("POS");
  display.setCursor(30, 20);
  display.println(data.xpos / 10);
  display.setCursor(60, 20);
  display.println(data.ypos / 10);
  display.setCursor(90, 20);
  display.println(data.zpos / 10);

  // writing stepper position
  display.setCursor(0, 35);
  display.println("STEPPER");
  display.setCursor(50, 35);
  display.println(data.zposStepper);
  display.setCursor(100, 35);
  display.println(data.stepperInc);

  // writing current
  display.setCursor(0, 50);
  display.println("CURRENT (pA)");
  display.setCursor(90, 50);
  display.println(data.current);

  display.display();
}



int checkVoltageStatus(int originalStatus) {
  // check from least important -> most important, returns error code
  int status = originalStatus;

  int vmon10val = analogRead(VMON10);
  int vmon5val  = analogRead(VMON5);
  int vmon33val = analogRead(VMON33);
  /*
    Serial.print("10V:");
    Serial.print(vmon10val);
    Serial.print(" 5V:");
    Serial.print(vmon5val);
    Serial.print(" 33V:");
    Serial.println(vmon33val);*/

  if (
    vmon10val > (VMON10EXPECTED + ALLOWEDRAILVARIANCE)
    || vmon10val < (VMON10EXPECTED - ALLOWEDRAILVARIANCE)
  )
  {
    status = 1;
  }
  else if (
    vmon5val > (VMON5EXPECTED + ALLOWEDRAILVARIANCE)
    || vmon5val < (VMON5EXPECTED - ALLOWEDRAILVARIANCE)
  )
  {
    status = 2;
  }
  else if (
    vmon33val > (VMON33EXPECTED + ALLOWEDRAILVARIANCE)
    || vmon33val < (VMON33EXPECTED - ALLOWEDRAILVARIANCE)
  )
  {
    status = 3;
  }

  return status;
}

float calibrated_nocurrent = 5625.0;

void plotBarsLog(int value) {
  // Expects a 16-bit value from 5957 t0 2^16-1, maps to 24 bars
  // Changes bar color from yellow (LOWCURRENT) to green to red (HIGHCURRENT)

  double num_bars_double = ((log10 ( ((double)value - calibrated_nocurrent)) - 2.7 ) * 12.0);
  int num_bars = round(num_bars_double);

  if (num_bars < 1) num_bars = 1;
  if (num_bars > 25) num_bars = 25;

  // updating display
  if (value < LOWCURRENT) for (int b = 0; b < num_bars; b++) bar.setBar(24 - b, LED_YELLOW);
  else if (value > HIGHCURRENT) for (int b = 0; b < num_bars; b++) bar.setBar(24 - b, LED_RED);
  else for (int b = 0; b < num_bars; b++) bar.setBar(24 - b, LED_GREEN);

  for (int b = num_bars; b < 25; b++) bar.setBar(24 - b, LED_OFF);
  bar.writeDisplay();

}

positionData initialApproach() {
  /*
      1) advance steppers until current detected, then stop steppers
      2) advance piezos until reach desired current

      3) hold piezos for ~1000 samples to get noise distribution
      4) retract piezos to 60000 while dumping data to ram
      5) dump data from RAM (to analyze exponential)
      6) re-extend piezos TODO
  */

  setPiezo(0, 39000);

  positionData position;

  position.xpos = 0;
  position.ypos = 0;
  position.zpos = 60000;
  position.zposStepper = 0;

  // TEST CODE START

  /*Serial.println("STARTING X-ax SCAN");

  int zpos_arr[1000];
  int curr_arr[1000];

  int orig_xpos = position.xpos;

  int counter = 0;

  for (int xpos = orig_xpos; xpos < orig_xpos + 1000; xpos++) {

    position.xpos = xpos;

    // repeat code starting here (not including dump to RAM)
    int currentRaw = readTIA();
    int stepIncrement = (currentRaw - DESIREDCURRENT) / 200;
    position.zpos += stepIncrement;

    setPiezoXYZ(position.xpos, position.ypos, position.zpos);
    // ending here

    int current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA
    if (counter % 100 == 0) {
        plotBarsLog(currentRaw);
        displayData data = {current, position.xpos, position.ypos, position.zpos, position.zposStepper, 0, checkVoltageStatus(5)};
        drawDisplay(data);
    }

      int xindex = xpos - orig_xpos;

      zpos_arr[xindex] = position.zpos;
      curr_arr[xindex] = current;

    counter += 1;
    delay(1);
  }*/

  // TEST CODE STOP

  // step down until we reach LOWCURRENT

  int currentRaw = readTIA();
  int current    = 0;

  // initial stepper approach

  setPiezoXYZ(position.xpos, position.ypos, position.zpos);

  //while (currentRaw < LOWCURRENT) {
  while (1) {

    int stepSpeed = enc.read();

    setupSteppers(abs(stepSpeed));

    if (abs(stepSpeed) > 10) {
      int stepIncrement = stepSpeed / abs(stepSpeed);
      moveStepperSteps(stepIncrement);
      position.zposStepper += stepIncrement;
    }
    currentRaw = readTIA();
    //int currentRaw = analogRead(A9) * 64; //readTIA();
    current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA
    plotBarsLog(currentRaw);

    displayData data = {current, position.xpos, position.ypos, position.zpos, position.zposStepper, stepSpeed, checkVoltageStatus(4)};
    drawDisplay(data);

    //Serial.print("stepspeed:");
    //Serial.print(stepSpeed);
    //Serial.print(",");
    //Serial.print(",stepperpos:");
    //Serial.print(position.zposStepper);
    //Serial.print(",");
    //Serial.print(",current:");
    //Serial.println(current);

    if (digitalRead(NEXTSW) == 0) break;

  }

  Serial.println("AUTO APPROACH");

  // then, approach with the piezos

  if (currentRaw > HIGHCURRENT) {
    Serial.println("OVERCURRENT!!!!");
    while (currentRaw > LOWCURRENT) {
      setupSteppers(300);
      moveStepperSteps(-1);
      position.zposStepper -= 1;
      currentRaw = readTIA();
    }
  }


  int counter = 0;

  while (currentRaw < DESIREDCURRENT) {
    // increment all piezo channels

    counter += 1;

    if (position.zpos < 10000) {
      position.zpos = 60000;
      setPiezoXYZ(position.xpos, position.ypos, position.zpos);
      setupSteppers(10);
      moveStepperSteps(5);
      position.zposStepper += 5;

    }


    int stepIncrement = (currentRaw - DESIREDCURRENT) / 200;
    if (abs(stepIncrement) > MAXSTEPINCREMENT) stepIncrement = stepIncrement / abs(stepIncrement) * MAXSTEPINCREMENT;

    position.zpos += stepIncrement;

    setPiezoXYZ(position.xpos, position.ypos, position.zpos);

    currentRaw = readTIA();


    if (counter % 100 == 0) {

      current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA
      plotBarsLog(currentRaw);

      displayData data = {current, position.xpos, position.ypos, position.zpos, position.zposStepper, 0, checkVoltageStatus(5)};
      drawDisplay(data);

      Serial.print(position.zposStepper / 10);
      Serial.print(",");
      Serial.print(position.zpos / 100);
      Serial.print(",");
      Serial.println(current);

    }
    delayMicroseconds(500);
  }


  // retract piezos while logging current

  /*Serial.print("retracting piezos from ");
  Serial.println(position.zpos);

  int numPoints = 60000-position.zpos;

  int zpos_retract_currents[numPoints];
  int zpos_approach_done = position.zpos;

  for(int i = zpos_approach_done; i < 60000; i++) {
    position.zpos = i;
    currentRaw = readTIA();
    setPiezoXYZ(position.xpos, position.ypos, position.zpos);
    current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA
    zpos_retract_currents[i-zpos_approach_done] = current;
    delay(1);
  }

  Serial.println("retracted piezos. Dumping data");

  for(int i = 0; i < numPoints; i++) {
    Serial.println(zpos_retract_currents[i]);
  }

  Serial.println("Data dumped");
  */

  // Move piezos to constant-current position

  Serial.println("Returning tip to constant-current position");

  counter = 0;
  while(currentRaw < DESIREDCURRENT) {
    currentRaw = readTIA();
    int stepIncrement = (currentRaw - DESIREDCURRENT) / 200;
    if (abs(stepIncrement) > MAXSTEPINCREMENT) stepIncrement = stepIncrement / abs(stepIncrement) * MAXSTEPINCREMENT;
    position.zpos += stepIncrement;
    setPiezoXYZ(position.xpos, position.ypos, position.zpos);
    current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA
    delayMicroseconds(500);
    counter += 1;


    if (counter % 100 == 0) {
      plotBarsLog(currentRaw);
      displayData data = {current, position.xpos, position.ypos, position.zpos, position.zposStepper, 0, checkVoltageStatus(5)};
      drawDisplay(data);
    }

  }

  // monitor current for 10000 cycles while holding Z constant

  Serial.println("Logging current while holding tip constant");

  int current_tip_constant[10000];

  counter = 0;
  for(int i = 0; i < 10000; i++) {
    currentRaw = readTIA();
    current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA
    current_tip_constant[i] = current;
    delayMicroseconds(500);

    if (counter % 100 == 0) {
      plotBarsLog(currentRaw);
      displayData data = {current, position.xpos, position.ypos, position.zpos, position.zposStepper, 0, checkVoltageStatus(5)};
      drawDisplay(data);
    }

    counter += 1;
  } 

  Serial.println("Finished logging current while holding tip constant. Dumping data");

  for(int i = 0; i < 10000; i++) {
    Serial.println(current_tip_constant[i]);
  }

  Serial.println("Finished dumping data");

  // Move piezos to constant-current position

  Serial.println("Moving to constant-current position");

  counter = 0;
  while(currentRaw < DESIREDCURRENT) {
    currentRaw = readTIA();
    int stepIncrement = (currentRaw - DESIREDCURRENT) / 200;
    if (abs(stepIncrement) > MAXSTEPINCREMENT) stepIncrement = stepIncrement / abs(stepIncrement) * MAXSTEPINCREMENT;
    position.zpos += stepIncrement;
    setPiezoXYZ(position.xpos, position.ypos, position.zpos);
    current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA
    delayMicroseconds(500);
    counter += 1;


    if (counter % 100 == 0) {
      plotBarsLog(currentRaw);
      displayData data = {current, position.xpos, position.ypos, position.zpos, position.zposStepper, 0, checkVoltageStatus(5)};
      drawDisplay(data);
    }

  }

  // monitor current for 10000 cycles while controlling Z

  Serial.println("Monitoring current and position while controlling Z");

  int current_tip_controlled[10000];
  int position_tip_controlled[10000];

  counter = 0;
  for(int i = 0; i < 10000; i++) {
    currentRaw = readTIA();
    int stepIncrement = (currentRaw - DESIREDCURRENT) / 200;
    if (abs(stepIncrement) > MAXSTEPINCREMENT) stepIncrement = stepIncrement / abs(stepIncrement) * MAXSTEPINCREMENT;
    position.zpos += stepIncrement;
    setPiezoXYZ(position.xpos, position.ypos, position.zpos);
    current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA

    current_tip_controlled[i] = current;
    position_tip_controlled[i] = position.zpos;
    delayMicroseconds(500);
    counter += 1;


    if (counter % 100 == 0) {
      plotBarsLog(currentRaw);
      displayData data = {current, position.xpos, position.ypos, position.zpos, position.zposStepper, 0, checkVoltageStatus(5)};
      drawDisplay(data);
    }

  }

  Serial.println("Finished monitoring current and position whlie controlling Z. Dumping data");

  for(int i = 0; i < 10000; i++) {
    Serial.print(current_tip_controlled[i]);
    Serial.print(",");
    Serial.println(position_tip_controlled[i]);
  }

  Serial.println("Finished dumping data. Starting X scan");

  return position;

}

void scanX(positionData position) {

/*
  TODO: Take (and print) 10 samples for every iteration (w 100us delay to capture noise)
*/

  Serial.println("STARTING X-ax SCAN");

  int zpos_arr[1000];
  int curr_arr[1000];

  int orig_xpos = position.xpos;

  int counter = 0;

  for (int xpos = orig_xpos; xpos < orig_xpos + 1000; xpos++) {
    
    position.xpos = xpos;

    // repeat code starting here (not including dump to RAM)
    int currentRaw = readTIA();
    int stepIncrement = (currentRaw - DESIREDCURRENT) / 200;
    position.zpos += stepIncrement;

    setPiezoXYZ(position.xpos, position.ypos, position.zpos);
    // ending here

    int current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA
    if (counter % 100 == 0) {
        plotBarsLog(currentRaw);
        displayData data = {current, position.xpos, position.ypos, position.zpos, position.zposStepper, 0, checkVoltageStatus(5)};
        drawDisplay(data);
    }

      int xindex = xpos - orig_xpos;

      zpos_arr[xindex] = position.zpos;
      curr_arr[xindex] = current;

    counter += 1;
    delayMicroseconds(500);
  }

  Serial.println("SCAN DIR 1 COMPLETE");
  for (int x = 0; x < 1000; x++) {
      Serial.print(x);
      Serial.print(",");
      Serial.print(zpos_arr[x]);
      Serial.print(",");
      Serial.println(curr_arr[x]);
  }

  int new_xpos = position.xpos;

  for (int xpos = new_xpos; xpos > orig_xpos; xpos--) {
    
    position.xpos = xpos;
    int currentRaw = readTIA();
    int stepIncrement = (currentRaw - DESIREDCURRENT) / 200;
    position.zpos += stepIncrement;

    setPiezoXYZ(position.xpos, position.ypos, position.zpos);

    int current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA

    if (counter % 100 == 0) {
        plotBarsLog(currentRaw);
        displayData data = {current, position.xpos, position.ypos, position.zpos, position.zposStepper, 0, checkVoltageStatus(5)};
        drawDisplay(data);
    }

      int xindex = xpos - orig_xpos - 1;

      zpos_arr[xindex] = position.zpos;
      curr_arr[xindex] = current;

    counter += 1;
    delayMicroseconds(500);
  }

  Serial.println("SCAN DIR 2 COMPLETE");
  for (int x = 0; x < 1000; x++) {
      Serial.print(x);
      Serial.print(",");
      Serial.print(zpos_arr[x]);
      Serial.print(",");
      Serial.println(curr_arr[x]);
  }
}

void loop() {
  /*

    //Serial.println("Scanning DAC");
    for (int i = 0; i < 30000; i += 100) {
      int currentRaw = readTIA();
      int current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA

      setPiezoXYZ(i, i, i);
      //Serial.print(i);
      //Serial.print(",");
      //Serial.print(currentRaw);
      //Serial.print(",");
      Serial.println(current);
      displayData data = {current, i, i, i, 0, checkVoltageStatus(0)};
      drawDisplay(data);
      plotBarsLog(currentRaw);


      //delay(10);
    }*/


  Serial.println("Starting Approach");
  positionData position = initialApproach();
  for (int i = 0; i < 100; i++) scanX(position);
  setPiezoXYZ(0, 0, 0);

  Serial.println("scan complete");
  delay(10000);


  /*
    int currentRaw = analogRead(A9) * 64; //readTIA();
    int current = (int) ((float)currentRaw - calibrated_nocurrent) * 3.3 / 65536.0 * 10000.0; //in pA
    displayData data = {current, 0, 0, 0, 0, checkVoltageStatus(0)};
    plotBarsLog(currentRaw);
    drawDisplay(data);
    moveStepperSteps(1);
    delay(10); */
}
