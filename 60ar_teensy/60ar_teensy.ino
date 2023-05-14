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
const unsigned int RFID_CHECK_TIMEOUT = 2500;  // ms
unsigned int rfid_timeout_check = 0;

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

bool checkID(unsigned int id) {
  // bool user_from_http = false;

  String tag_str = String(id);
  while (tag_str.length() < 10) {
    tag_str = "0" + tag_str;
  }
  char tag_char[11];
  tag_str.toCharArray(tag_char, 11);
  tag_str[sizeof(tag_str) - 1] = '\0';

  Serial.print("Checking ID: ");
  Serial.println(tag_char);

  File file = SD.open(filename);
  if (file) {
    Serial.println(F("Opening file..."));
  } else {
    Serial.println(F("Error opening SD card"));
    return false;
  }

  StaticJsonDocument<13> doc;

  // TODO GET/:userid fallback
  // user_from_http = true;

  // eg. https://arduinojson.org/v6/how-to/deserialize-a-very-large-document/
  unsigned long start = millis();
  while (file.available()) {
    if (!file.find("\"ids\": [")) {
      Serial.println(F("File-find error"));
      return false;
    }
    while (file.peek() != ']') {
      if (file.peek() == ',' || file.peek() == '[') {
        file.read(); // skip comma or opening bracket
        continue;
      }
      DeserializationError desz_json_err = deserializeJson(doc, file);
      if (desz_json_err) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(desz_json_err.c_str());
        return false;
      }

      // tags are read in with quotes
      char buffer[12]; // " + 10 num + "
      serializeJsonPretty(doc, buffer, 12);
      char buffer_num[11];
      snprintf(buffer_num, 11, "%.*s", 10, &buffer[1]);

      if (strcmp(buffer_num, tag_char) == 0) {
        Serial.println(F("Found valid ID!"));
        return true;
      }
    }
    Serial.println(F("No matching ID found"));
    Serial.print(F("Elapsed time (ms): "));
    Serial.println(millis() - start);
    return false;
  }

  // check user
  // if user_from_http, update SD card
  return false;
}

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
  while (!Serial)
    ;


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
  // test relay
  // digitalWrite(pin_RELAY, HIGH);
  // delay(5000);
  // digitalWrite(pin_RELAY, LOW);
}

void loop() {
  unsigned int found_tag = readRFID();
  unsigned long now = millis();

  if (found_tag && (now - rfid_timeout_check >= RFID_CHECK_TIMEOUT)) {
    // prevent multi fires, convert to mills check
    RFID_SERIAL.clear();
    rfid_timeout_check = now;
    Serial.print(F("Good tag, sleeping for: "));
    Serial.println(RFID_CHECK_TIMEOUT);

    checkID(found_tag);
  }
  delay(100);
}