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
#include "player.h"
#include "song.h"

/** FOR TEENSY 4.1 */

// File must be formatted JSON and alike { "url": "www.foo.com/user/ids", "ids": [] }
static const char *filename = "ids.txt";  // 8+3 filename max

static const unsigned int BAUD_RATE = 115200;
static const unsigned int RFID_CHECK_TIMEOUT = 2500;  // ms

static const byte pin_DATA = 13;
static const byte pin_SUCCESS = 14;
static const byte pin_ERROR = 15;
static const byte pin_RELAY = 33;

volatile unsigned int rfid_timeout_check = 0;

static byte mac[] = { 0x04, 0xE9, 0xE5, 0x14, 0xDD, 0x62 };
bool has_http_access = false;
AsyncHTTPRequest request;
char user_id_url[256];

#include "check_id.h"

// SETUP Fns
void initEthernet() {
  Ethernet.setMACAddress(mac);
  Ethernet.begin();
  if (Ethernet.waitForLocalIP(2000)) {
    Serial.print(F("Available at: "));
    Serial.println(Ethernet.localIP());
    has_http_access = true;
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


void initLights() {
  pinMode(pin_DATA, OUTPUT);
  pinMode(pin_SUCCESS, OUTPUT);
  pinMode(pin_ERROR, OUTPUT);
}

void checkLights() {
  analogWrite(pin_DATA, 255);
  delay(250);
  analogWrite(pin_DATA, 0);
  delay(1);
  analogWrite(pin_SUCCESS, 255);
  delay(250);
  analogWrite(pin_SUCCESS, 0);
  delay(1);
  analogWrite(pin_ERROR, 255);
  delay(250);
  analogWrite(pin_ERROR, 0);
}

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial)
    ;

  play_song(melody, sizeof(melody)/sizeof(melody[0])/2);

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
  unsigned int read_tag = readRFID();
  unsigned long now = millis();

  if (read_tag && (now - rfid_timeout_check >= RFID_CHECK_TIMEOUT)) {
    RFID_SERIAL.clear();
    Serial.print(F("Read tag, resetting timeout."));
    rfid_timeout_check = now;

    checkID_SD(read_tag);
    // bool tag_on_SD = checkID_SD(read_tag);
    // if (!tag_on_SD && has_http_access && strln(user_id_url) > 0) {
      // Serial.println("Checking for tag at provided URL")
      // TODO GET/:userid fallback
      // & Update SD
    // }

  }
  delay(100);
}