#include "DHT.h"
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <SPI.h>
#include <Ethernet.h>

// Ether
byte mac[] = {0xA8, 0x61, 0x8A, 0xAE, 0x96, 0x64};

IPAddress ip(192, 168, 1, 177);
IPAddress dns(192, 168, 1, 1);

EthernetClient client;

// Sensor
DHT dht(2, DHT11);

// LCD
TwoWire Wire_1 = TwoWire();
LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27

// timing
const unsigned long interval_sensor = 2000; // ms
unsigned long prevTs = 0;

// timing dot
bool dot = false;
const unsigned long interval_dot = 500; // ms
unsigned long prevTd = 0;


template<class T>
void printOnly(T str, int dly = 2000) {
  lcd.clear();
  lcd.home();
  Serial.println(str);
  lcd.print(str);
  delay(dly);
}

void printDHT(float humi, float tempC, float tempF) {
  Serial.print("H: ");
  Serial.print(humi);
  Serial.print("%");

  Serial.print("  |  ");

  Serial.print("T: ");
  Serial.print(tempC);
  Serial.print("C - ");
  Serial.print(tempF);
  Serial.println("F");

  lcd.clear();
  lcd.home();
  lcd.print("H: ");
  lcd.print(humi, 2);
  lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("T: ");
  lcd.print(tempC, 1);
  lcd.print("C - ");
  lcd.print(tempF, 1);
  lcd.print("F");
}

bool handleDHT() {
  float humi  = dht.readHumidity();
  float tempC = dht.readTemperature();
  float tempF = dht.readTemperature(true);
  byte invalid = isnan(humi) || isnan(tempC) || isnan(tempF);
  // check if any reads failed
  if (invalid ) {
    Serial.print("humi ");
    Serial.println(isnan(humi));
    Serial.print("tempC ");
    Serial.println(isnan(tempC));
    Serial.print("tempF ");
    Serial.println(isnan(tempF));
    Serial.println(F("Failed to read sensor!"));
    lcd.clear();
    lcd.print(F("sensor read err"));
  } else {
    printDHT(humi, tempC, tempF);
  }
  return !invalid;
}

void handleDot() {
  lcd.setCursor(15, 0);
  lcd.print(dot ? ' ' : '.');
  dot = !dot;
}

void initEther() {
  if (!Ethernet.begin(mac, 2500)) {
    if (!Ethernet.hardwareStatus()) {
      printOnly(F("bad ether hardware"));
      return;
    }
    if (Ethernet.linkStatus() == LinkOFF) {
      printOnly(F("no link"));
      return;
    }

    printOnly(F("DHCP err"));
    Ethernet.begin(mac, ip, dns);
  }

  IPAddress ip = Ethernet.localIP();
  Serial.println(ip);
  if (ip) {
    Serial.print("Local IP: ");
    printOnly(ip);
  } else {
    Serial.println(F("no IP"));
    printOnly(F("no IP"));
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // init LCD
  Wire_1.begin();
  Wire_1.setClock(100000); // Hz+
  lcd.begin(16, 2, Wire_1);
  lcd.print(F("Hello world"));
  lcd.setBacklight(HIGH);
  initEther();

  dht.begin();

}

void loop() {
  // wait a few seconds between measurements.
  unsigned long currT = millis();

  if (currT - prevTd >= interval_dot) {
    handleDot();
    prevTd = currT;
  }


  if (currT - prevTs >= interval_sensor) {
    if(handleDHT()) {
      dot = true;
      // serveData
      // storeData
    }
    prevTs = currT;
  }

}
