#include <SPI.h>
// Ethernet
#include "QNEthernet.h"
#include <AsyncHTTPRequest_Teensy41.h>
// SD Card
#include <SD.h>
#include <ArduinoJson.h>

using namespace qindesign::network;

AsyncHTTPRequest request;
byte mac[] = {};

// File must be of type { "url": "www.foo.com/user/ids", "ids": [] }
const char *filename = "/id.txt";  // 8+3 filename max

char* user_id_url;

void teensyMAC(uint8_t *mac) {
    Serial.println("Set MAC and refactor");
    for(uint8_t by=0; by<2; by++) mac[by]=(HW_OCOTP_MAC1 >> ((1-by)*8)) & 0xFF;
    for(uint8_t by=0; by<4; by++) mac[by+2]=(HW_OCOTP_MAC0 >> ((3-by)*8)) & 0xFF;
    Serial.printf("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void initEthernet() {

  if(Ethernet.begin() == 0) {
    Serial.println(F("Failed to link via DHCP"));
    Serial.println(F("Can only validate against existing IDs"));
  } else {
    Serial.print(F("Connected at IP: "));
    Serial.println(Ethernet.localIP());
  }
}

void initSD() {
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println(F("Critical error - cannot read SD Card"));
    while(1);
  } else {
    Serial.println(F("SD Card access OK"));
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

void loadUserURL() {
  StaticJsonDocument<64> doc;
  StaticJsonDocument<64> filter;
  filter["url"] = true;

  // TODO input...

  DeserializationError error = deserializeJson(doc, input, DeserializationOption::Filter(filter));
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  user_id_url = doc["url"];
}

bool checkID(unsigned int id) {
  bool user_from_http = false;
  
  StaticJsonDocument<1024> doc;
  StaticJsonDocument<64> filter;
  filter["ids"] = true;

  File file = SD.open(filename);
  if (!file) {
    Serial.println(F("Error opening SD card to check user"));
    return;
  }

  // TODO GET/:userid fallback
  // user_from_http = true;

  // eg. https://arduinojson.org/v6/how-to/deserialize-a-very-large-document/
  while (file.available()) {
    DeserializationError error = deserializeJson(doc, input, DeserializationOption::Filter(filter));
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
  }

  // check user
  // if user_from_http, update SD card

}

void setup() {
  Serial.begin(115200);
  while(!Serial);
  teensyMAC(mac);

  initEthernet();
  initSD();
  checkSD();
  loadUserURL()
}

void loop() {
  // listen for RFID

}