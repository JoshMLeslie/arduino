#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <EEPROM.h>

const bool DEBUG = false;

unsigned long past = 0L;

// per label on old speedo,
// 2240 revs = 1 mile
volatile unsigned long count = 0L;
float dist = 16837.9; // 16337.9 miles from old speedo, adj with apprx. since removal
unsigned long idle_timeout = 0L; // save to EEPROM after timeout;

// LCD
TwoWire Wire_1 = TwoWire();
LiquidCrystal_PCF8574 lcd(0x27);

// Fns
// interrupt service routine
void isr() {
  count++;
}

unsigned int mphMath() {
  unsigned long rps = count;
  unsigned long rpm = 60L * rps;
  unsigned long rph = 60L * rpm;
  return (float)rph / 2240;
}

void debugPrint() {
  unsigned long rps = count;
  unsigned long rpm = 60L * rps;
  unsigned long rph = 60L * rpm;
  unsigned int mph = (float)rph / 2240;
  Serial.println("__poll__");
  Serial.print("count: ");
  Serial.print(count);
  Serial.print(" elapsed: ");
  Serial.println("1000");
  Serial.print("rps: ");
  Serial.print(rps);
  Serial.print(" rpm: ");
  Serial.print(rpm);
  Serial.print(" rph: ");
  Serial.print(rph);
  Serial.print(" mph: ");
  Serial.println(mph);
}

// reset + print display only at output
void lcdPrint(unsigned int mph) {
  lcd.setCursor(5, 0);
  lcd.print("   ");
  lcd.setCursor(5, 0);
  lcd.print(mph);
}

void distUpdate(float mps) {
  if (mps > 0 && mps < 0.001) {
    // noise
    return;
  }

  unsigned long dTs = millis();
  float newDist = (float)dist + mps;
  bool updateDist = newDist != dist;

  if (updateDist) {
    lcd.setCursor(6, 1);
    lcd.print("          ");
    lcd.setCursor(6, 1);
    lcd.print(dist, 1);
  }

  if (mps == 0.00 && idle_timeout == 0) {
    Serial.print("timeout set: ");
    Serial.println(dTs);
    idle_timeout = dTs;
  } else if (mps > 0 && idle_timeout != 0) {
    idle_timeout = 0;
  }

  // save the EEPROM from tons of writes
  // or we've been idle for >30 seconds
  if (
    newDist > dist + 5 ||
    (
      updateDist &&
      idle_timeout != 0 &&
      dTs - idle_timeout >= 30000
    )
  ) {
    EEPROM.put(0, newDist);
    idle_timeout = 0;
  } else {
    dist += mps;
  }
}

// SETUP
void lcdSetup() {
  Wire_1.begin();
  Wire_1.setClock(100000); // Hz
  lcd.begin(16, 2, Wire_1);
  lcd.print(F("Hello world"));
  lcd.setBacklight(HIGH);
  delay(1000);
  lcd.clear();
  lcd.home();
  lcd.print("MPH: ");
  lcd.setCursor(0, 1);
  lcd.print("Dist: ");
  lcd.print(dist, 1);
} 

void distSetup() {
  EEPROM.get(0, dist);
}

void BIOS() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println(F("booting"));
}

void setup() {
  BIOS();

  attachInterrupt(0, isr, RISING);

  distSetup();
  lcdSetup();

  // IR LED
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
}

void loop() {
  unsigned long tMs = millis();

  if (tMs - past >= 1000) {
    if (DEBUG) {
      debugPrint();
    } else {
      unsigned int mph = mphMath();
      lcdPrint(mph);
      float mps = (float)mph / 3600;

      distUpdate(mps);
    }
    past = tMs;
    count = 0L;
  }
}
