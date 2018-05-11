// csStepControl
//
// v0.5 -- works with any adafruit feather with one UART and SPI. 
// -- also designed to work with an oled featherwing.
//
// Allows for stepper mottor control in behavioral tasks.
// Makes use of:
// a) the AccelStepper library by Mike McCauley (mikem@airspayce.com)
// b) the Trinamic 2130 SPI interface and a wrapper to it by:
// if you do not want to use the trinamic driver, use csStepControlAgnostic

// this is state based
// s0 is the rest/report state
// s1 is a simple step state that provides one direction for N number of steps.
// s2 is a continuous state, which will ramp up to the speed you want and stay there.

// it will report various things in a csv serial write with a header of csStep


#include <SPI.h>
#include <AccelStepper.h>
#include <TMC2130Stepper.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 5
#define CS_PIN 10
#define EN_PIN 2 //enable (CFG6)
#define DIR_PIN 23 //direction
#define STEP_PIN 22 //step
#define MOSI_PIN  12
#define MISO_PIN 11
#define SCK_PIN  13

#define s1Pin 6
#define s2Pin 7


int dirDebounce = 0;


//Trinamic_TMC2130 myStepper(CS_PIN);

TMC2130Stepper driver = TMC2130Stepper(EN_PIN, DIR_PIN, STEP_PIN, CS_PIN, MOSI_PIN, MISO_PIN, SCK_PIN);
Adafruit_SSD1306 display(OLED_RESET);

// m=microSteps; t=totalSteps; d='direction'; s=speed; a=acel; z=state;
char knownHeaders[] = {'m', 't', 'd', 's', 'a', 'z', 'p', 'b'};
float knownValues[] = {256, 50, 1, 100, 10000, 0, 0, 2};
float defaultSet[] = {256, 50, 1, 100, 10000, 0, 0, 2};
int knownCount = 8;
float volPerStep = 2.529423872607;
float volPerMu = volPerStep / knownValues[0];

int stateHeaders[] = {0, 0, 0, 0};
int screenUpdate = 0;
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN); // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5


void createHomeScreen() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("s:");
  display.print(int(knownValues[2]*knownValues[3]*knownValues[0]));
  display.setCursor(60, 0);
  display.print("mode:");
  if (knownValues[7] == 1) {
    display.print("single");
  }
  else {
    display.print("cont.");
  }
  display.setCursor(0, 10);
  display.print("v:");
  display.print(stepper.currentPosition()*volPerMu);
  display.setCursor(70, 10);
  display.print("t:");
  if (knownValues[7] == 2) {
    display.print("inf");
  }
  else {
    display.print(int(knownValues[1]*knownValues[2]));
  }

  display.setCursor(0, 22);
  display.setTextColor(BLACK, WHITE);
  display.print("A=");
  display.print("run");
  display.print("  B=");
  display.print("dir");
  display.print(" C=");
  display.print("set");
  display.display();
  display.setTextColor(WHITE);

}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  delay(2000);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  //  display.setTextColor(BLACK, WHITE); // 'inverted' text

  pinMode(4, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(6, INPUT_PULLDOWN);
  pinMode(7, INPUT_PULLDOWN);


  driver.begin();

  driver.rms_current(600);  // Set stepper current to 600mA. The command is the same as command TMC2130.setCurrent(600, 0.11, 0.5);
  driver.stealthChop(1);  // Enable extremely quiet stepping
  Serial.begin(9600);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, LOW);
  stepper.setMaxSpeed(2000 * knownValues[0]);
  stepper.setSpeed(knownValues[3]*knownValues[0]);
  stepper.setAcceleration(knownValues[4]*knownValues[0]);
  knownValues[6] = stepper.currentPosition();
  digitalWrite(EN_PIN, HIGH);
  createHomeScreen();

}

void loop() {
  static bool bStates[] = {1, 1, 1, 0, 0};
  static bool stepLatch = 0;
  static bool dirLatch = 0;
  static bool setLatch = 0;
  static bool s1Latch = 0;
  static bool s2Latch = 0;


  // ********** Handle Buttons and Input Triggers
  bStates[0] = digitalRead(4);
  bStates[1] = digitalRead(3);
  bStates[2] = digitalRead(8);
  bStates[3] = digitalRead(6);
  bStates[4] = digitalRead(7);

  if ((bStates[3] == 1) && (s1Latch == 0)) {
    s1Latch = 1;
    knownValues[5] = 1;
  }

  if ((s1Latch == 1) && (bStates[3] == 0)) {
    s1Latch = 0;

  }

  if ((bStates[4] == 1) && (s2Latch == 0)) {
    s2Latch = 1;
    knownValues[5] = 2;
  }

  if ((s2Latch == 1) && (bStates[4] == 0)) {
    s2Latch = 0;

  }

  if ((bStates[0] == 0) && (stepLatch == 0)) {
    stepLatch = 1;
    knownValues[5] = knownValues[7] - knownValues[5];
  }

  if ((stepLatch == 1) && (bStates[0] == 1)) {
    stepLatch = 0;

  }

  if ((bStates[1] == 0) && (dirLatch == 0)) {
    knownValues[2] = -1 * knownValues[2];
    Serial.println(knownValues[2]);
    dirLatch = 1;
  }

  if ((dirLatch == 1) && (bStates[1] == 1)) {
    dirLatch = 0;
  }

  if ((bStates[2] == 0) && (setLatch == 0)) {
    setLatch = 1;
    for ( int i = 0; i < knownCount; i++) {
      knownValues[i] = defaultSet[i];
    }
    stepper.setCurrentPosition(0);
  }


  if ((setLatch == 1) && (bStates[2] == 1)) {
    setLatch = 0;
  }


  if (screenUpdate >= 500) {
    createHomeScreen();
    screenUpdate = 0;

  }

  static float baseTime;
  flagReceive(knownHeaders, knownValues);

  // step state
  if (knownValues[5] == 1) {
    if (stateHeaders[1] == 0) {
      digitalWrite(EN_PIN, LOW);
      baseTime = millis();
      stepper.setMaxSpeed(2000 * knownValues[0]);
      stepper.setSpeed(knownValues[3]*knownValues[0]);
      stepper.setAcceleration(knownValues[4]*knownValues[0]);
      stepper.move(knownValues[1]*knownValues[2]);
      stateHeaders[0] = 0;
      stateHeaders[1] = 1;
      stateHeaders[2] = 0;
      stateHeaders[3] = 0;
    }
    stepper.run();
    if (stepper.isRunning() == 0) {
      stepper.stop();
      knownValues[5] = 0;
      digitalWrite(EN_PIN, HIGH); // disable motor
    }
  }

  // rest state
  else if (knownValues[5] == 0) {
    if (stateHeaders[0] == 0) {
      if (stepper.isRunning() == 0) {
        stepper.stop();
        digitalWrite(EN_PIN, HIGH); // disable motor
      }
      Serial.print("csStep,");
      Serial.print(int(millis() - baseTime));
      Serial.print(',');
      Serial.print(stepper.currentPosition());
      Serial.print(',');
      Serial.print(knownValues[3]);
      Serial.print(',');
      Serial.print(knownValues[4]);
      Serial.print(',');
      Serial.println(knownValues[0]);
      stateHeaders[0] = 1;
      stateHeaders[1] = 0;
      stateHeaders[2] = 0;
      stateHeaders[3] = 0;

    }
  }

  else if (knownValues[5] == 2) {
    if (stateHeaders[2] == 0) {
      stateHeaders[0] = 0;
      stateHeaders[1] = 0;
      stateHeaders[2] = 1;
      stateHeaders[3] = 0;
      stepper.setMaxSpeed(2000 * knownValues[0]);
      stepper.setSpeed(knownValues[2]*knownValues[3]*knownValues[0]);
      stepper.setAcceleration(knownValues[4]*knownValues[0]);
    }
    digitalWrite(EN_PIN, LOW);
    stepper.runSpeed();
  }

  // reset defaults state
  else if (knownValues[5] == 9) {
    if (stateHeaders[3] == 0) {
      stateHeaders[0] = 0;
      stateHeaders[1] = 0;
      stateHeaders[2] = 0;
      stateHeaders[3] = 1;
      knownValues[0] = 1;
      knownValues[1] = 200;
      knownValues[2] = 1;
      knownValues[3] = 100;
      knownValues[4] = 10000;
      knownValues[5] = 0;
    }
  }
  screenUpdate = screenUpdate + 1;
  dirDebounce = dirDebounce + 1;
}





int flagReceive(char varAr[], float valAr[]) {
  static boolean recvInProgress = false;
  static byte ndx = 0;
  char endMarker = '>';
  char feedbackMarker = '<';
  char rc;
  int nVal;
  const byte numChars = 32;
  char writeChar[numChars];
  static int selectedVar;
  int newData = 0;

  while (Serial.available() > 0 && newData == 0) {
    rc = Serial.read();
    if (recvInProgress == false) {
      for ( int i = 0; i < knownCount; i++) {
        if (rc == varAr[i]) {
          selectedVar = i;
          recvInProgress = true;
        }
      }
    }

    else if (recvInProgress == true) {
      if (rc == endMarker ) {
        writeChar[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = 1;

        nVal = int(String(writeChar).toInt());
        valAr[selectedVar] = nVal;

      }
      else if (rc == feedbackMarker) {
        writeChar[ndx] = '\0'; // terminate the string
        recvInProgress = false;
        ndx = 0;
        newData = 1;
        Serial.print("echo");
        Serial.print(',');
        Serial.print(varAr[selectedVar]);
        Serial.print(',');
        Serial.print(valAr[selectedVar]);
        Serial.print(',');
        Serial.print(selectedVar);
        Serial.println('~');
      }

      else if (rc != feedbackMarker || rc != endMarker) {
        writeChar[ndx] = rc;
        ndx++;
        if (ndx >= numChars) {
          ndx = numChars - 1;
        }
      }
    }
  }
  return newData; // tells us if a valid variable arrived.
}
