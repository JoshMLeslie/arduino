#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <EEPROM.h>

#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>

unsigned long past = 0L;

// LCD
TwoWire Wire_1 = TwoWire();
LiquidCrystal_PCF8574 lcd(0x27);

SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);
#define GPSECHO false

// unsigned int mphMath() {
//   unsigned long rps = count;
//   unsigned long rpm = 60L * rps;
//   unsigned long rph = 60L * rpm;
//   return (float)rph / 2240;
// }

// reset + print display only at output
void lcdPrint(unsigned int mph) {
  lcd.setCursor(5, 0);
  lcd.print("   ");
  lcd.setCursor(5, 0);
  lcd.print(mph);
}

// SETUP
void lcdSetup() {
  Wire_1.begin();
  Wire_1.setClock(100000);  // Hz
  lcd.begin(16, 2, Wire_1);
  lcd.print(F("Hello world"));
  lcd.setBacklight(HIGH);
  delay(1000);
  lcd.clear();
  lcd.home();
  // lcd.print("MPH: ");
  // lcd.setCursor(0, 1);
  // lcd.print("Dist: ");
  // lcd.print(dist, 1);
}

void distSetup() {
  // EEPROM.get(0, disno matching function for call to 'TwoWire::TwoWire()'t);
}

void BIOS() {
  Serial.begin(9600);
  while (!Serial)
    ;
  Serial.println(F("booting"));
}

void gpsSetup() {
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  // print firmware version
  Serial.print(F("GPS Firmware: "));
  Serial.println(PMTK_Q_RELEASE);
  delay(500);
}

void setup() {
  BIOS();

  // lcdSetup();
  gpsSetup();

  Serial.println(F("boot complete"));
}

void loop() {

  if (millis() - past > 1000) {

    lcd.clear();
    lcd.home();
  
    char c = GPS.read();

    lcd.print("Time: ");
    if (GPS.hour < 10) { lcd.print('0'); }
    lcd.print(GPS.hour, DEC);
    lcd.print(':');
    if (GPS.minute < 10) { lcd.print('0'); }
    lcd.print(GPS.minute, DEC);
    lcd.print(':');
    if (GPS.seconds < 10) { lcd.print('0'); }
    lcd.print(GPS.seconds, DEC);
    // off screen
    // lcd.print(' - ');
    // lcd.print("Date: ");
    // lcd.print(GPS.day, DEC);
    // lcd.print('/');
    // lcd.print(GPS.month, DEC);
    // lcd.print("/20");
    // lcd.print(GPS.year, DEC);

    lcd.setCursor(0, 1);

    lcd.print("Fx ");
    lcd.print((int)GPS.fix);
    lcd.print(" Q ");
    lcd.print((int)GPS.fixquality);
    lcd.print(" A ");
    lcd.print((int)GPS.antenna);

    if (GPS.fix) {
      delay(1000);
      lcd.clear();
      lcd.home();
      lcd.print("Location: ");
      lcd.print(GPS.latitude, 4);
      lcd.print(GPS.lat);
      lcd.print(", ");
      lcd.print(GPS.longitude, 4);
      lcd.print(GPS.lon);

      // lcd.print("Speed (knots): ");
      // lcd.print(GPS.speed);
      // lcd.print("Angle: ");
      // lcd.print(GPS.angle);
      // lcd.print("Altitude: ");
      // lcd.print(GPS.altitude);
      lcd.setCursor(0, 1);
      lcd.print("Sats: ");
      lcd.print((int)GPS.satellites);
    }

    past = millis();
  }
}
