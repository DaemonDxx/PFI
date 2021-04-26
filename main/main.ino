#include <EEPROM.h>
#include <arduino-timer.h>
#include <timer.h>

#define INT_PIN 2
#define OUT_PIN 5
#define LED_PIN 13
#define PERCENT 0.33
#define TIME_DELAY 200
#define BUTTON_PIN 8

#define TIME_FOR_TOGGLE_MODE 2000

unsigned long deltaTime = 0;
unsigned long timeToCatchSignal = 0;
unsigned long timeOffsetSignal = 0;
unsigned long lastTimePush = 0;

bool isDelayModeOn = false;
bool isUnhandledInterrupt = false;
bool isTimeToOff = false;

auto timer = timer_create_default();


void setup() {
  Serial.begin(115200);
  if (readModeFromEPPROM()) {
    delayModeOn();
  } else {
    delayModeOff();
  }
  pinMode(INT_PIN, INPUT);
  pinMode(OUT_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(0, interruptHandler, RISING);
  digitalWrite(OUT_PIN, HIGH);
  //Serial.println("Start program");
}

bool readModeFromEPPROM() {
  bool state = EEPROM.read(0);
  //Serial.print("Read state from EEPROM: ");
  //Serial.println(state);
  return state;
}

void saveModeInEPPROM(bool state) {
  EEPROM.write(0, state);
  //Serial.print("Save state in EEPROM: ");
  //Serial.println(state);
}

void loop() {
  timer.tick();
  if (isUnhandledInterrupt) {
    catchSignal();
    isUnhandledInterrupt = false;
  }
  if (isTimeToOff) {
    timer.cancel();
    timer.in(40, outOff
    );
    isTimeToOff = false;
  }
  if(!digitalRead(BUTTON_PIN)) {
    toogleMode();
    delay(1000);
  }
}

void checkToggleMode() {
    if (digitalRead(INT_PIN)) {
        toogleMode();
    }
}

void toogleMode() {
  if (isDelayModeOn) {
    delayModeOff();
    saveModeInEPPROM(false);
    Serial.print("MODE OFF");
  } else {
    delayModeOn();
    saveModeInEPPROM(true);
    Serial.println("MODE ON");
  }
}

void delayModeOn() {
  digitalWrite(LED_PIN, HIGH);
  isDelayModeOn = true;
  Serial.println("Delay mode: ON");
}

void delayModeOff() {
  digitalWrite(LED_PIN, LOW);
  isDelayModeOn = false;
  Serial.println("Delay mode: OFF");
}

void interruptHandler() {
  if (digitalRead(INT_PIN)) {
    if (millis() - lastTimePush > TIME_DELAY) {
      Serial.println("Catch!!");
      if (isDelayModeOn) {
        isUnhandledInterrupt = true;
      } else {
        timer.in(0, outOn);
        timer.in(50, outOff);
      }
      lastTimePush = millis();
    }
  }
}

void catchSignal() {
  if (timeToCatchSignal == 0) {
    timeToCatchSignal = millis();
    timer.in(0, outOn);
  } else {
    deltaTime = millis() - timeToCatchSignal;
    timeOffsetSignal = countDelay(deltaTime);
    timer.cancel();
    timer.in(timeOffsetSignal, outOn);
    Serial.println("Delta time: ");
    Serial.println(deltaTime);
    Serial.println("Time offsets: ");
    Serial.println(timeOffsetSignal);
    timeOffsetSignal = 0;
    timeToCatchSignal = 0;
    deltaTime = 0;
  }
}

double countDelay(unsigned long deltaTime) {
  return deltaTime / (1 - PERCENT) - deltaTime;
}

bool outOn(void *argument) {
  digitalWrite(OUT_PIN, HIGH);
  Serial.println("Out pin: ON");
  isTimeToOff = true;
  return false;
}

bool outOff(void *argument) {
  digitalWrite(OUT_PIN, LOW);
  Serial.println("Out pin: OFF");
  return false;
}
