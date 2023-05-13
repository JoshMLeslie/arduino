#include <SPI.h>
#include <ArduinoJson.h>
// Ethernet
#include "QNEthernet.h"
#include <AsyncHTTPRequest_Teensy41.h>
using namespace qindesign::network;
// SD Card
#include <SD.h>
// local files
#include "rfid.h"

/** FOR TEENSY 4.1 */

// File must be of type { "url": "www.foo.com/user/ids", "ids": [] }
const char *filename = "ids.txt";  // 8+3 filename max

const unsigned int BAUD_RATE = 115200;

const byte pin_DATA = 13;
const byte pin_SUCCESS = 14;
const byte pin_ERROR = 15;
const byte pin_RELAY = 33;

byte mac[] = { 0x04, 0xE9, 0xE5, 0x14, 0xDD, 0x62 };
AsyncHTTPRequest request;
char *user_id_url;

// SETUP Fns
void initEthernet() {
  Ethernet.setMACAddress(mac);
  Ethernet.begin();
  if (Ethernet.waitForLocalIP(2000)) {
    Serial.print(F("Available at: "));
    Serial.println(Ethernet.localIP());
  } else {
    Serial.println(F("Failed to link via DHCP"));
    Serial.println(F("Can only validate against stored IDs"));
  }
}

void initSD() {
  if (SD.begin(BUILTIN_SDCARD)) {
    Serial.println(F("SD Card access OK"));
  } else {
    Serial.println(F("Critical error - cannot read SD Card"));
    while (1)
      ;
  }
}

void checkSD() {
  File data_file = SD.open(filename);
  if (!data_file) {
    Serial.print(filename);
    Serial.println(F("not found"));
  }
  data_file.close();
}

// void loadUserURL() {
//   StaticJsonDocument<64> doc;
//   StaticJsonDocument<64> filter;
//   filter["url"] = true;

//   // TODO input...

//   DeserializationError error = deserializeJson(doc, input, DeserializationOption::Filter(filter));
//   if (error) {
//     Serial.print(F("deserializeJson() failed: "));
//     Serial.println(error.f_str());
//     return;
//   }

//   user_id_url = doc["url"];
// }
// end SETUP Fns

// bool checkID(unsigned int id) {
//   bool user_from_http = false;

//   StaticJsonDocument<1024> doc;
//   StaticJsonDocument<64> filter;
//   filter["ids"] = true;

//   File file = SD.open(filename);
//   if (!file) {
//     Serial.println(F("Error opening SD card to check user"));
//     return;
//   }

//   // TODO GET/:userid fallback
//   // user_from_http = true;

//   // eg. https://arduinojson.org/v6/how-to/deserialize-a-very-large-document/
//   while (file.available()) {
//     DeserializationError error = deserializeJson(doc, input, DeserializationOption::Filter(filter));
//     if (error) {
//       Serial.print(F("deserializeJson() failed: "));
//       Serial.println(error.f_str());
//       return;
//     }
//   }

//   // check user
//   // if user_from_http, update SD card
// }
void initLights() {
  pinMode(pin_DATA, OUTPUT);
  pinMode(pin_SUCCESS, OUTPUT);
  pinMode(pin_ERROR, OUTPUT);
}

void checkLights() {
  analogWrite(pin_DATA, HIGH);
  delay(250);
  analogWrite(pin_DATA, LOW);
  analogWrite(pin_SUCCESS, HIGH);
  delay(250);
  analogWrite(pin_SUCCESS, LOW);
  analogWrite(pin_ERROR, HIGH);
  delay(250);
  analogWrite(pin_ERROR, LOW);
}

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial);


  initLights();
  checkLights();

  // Init utilities
  pinMode(pin_RELAY, OUTPUT);
  initEthernet();
  initSD();
  checkSD();
  initRFID();
  
  // todo - load from SD card
  // loadUserURL();

  Serial.println(F("Init complete"));
}

void loop() {
  bool found_tag = readRFID();
  if (found_tag) {
    // prevent multi fires, convert to mills check
    RFID_SERIAL.clear();
    Serial.print(F("Good tag, sleeping for: "));
    Serial.println(RFID_CHECK_TIMEOUT);
    delay(RFID_CHECK_TIMEOUT);
  }
  delay(100);
}