#include <SPI.h>
#include <ArduinoJson.h>
// Ethernet
#include "QNEthernet.h"
#include <AsyncHTTPRequest_Teensy41.h>
using namespace qindesign::network;
// SD Card
#include <SD.h>
// RFID
#include <SoftwareSerial.h>

/** FOR TEENSY 4.1 */

// File must be of type { "url": "www.foo.com/user/ids", "ids": [] }
const char *filename = "ids.txt";  // 8+3 filename max

const byte BAUD_RATE = 115200;

const byte pin_DATA = 13;
const byte pin_SUCCESS = 14;
const byte pin_ERROR = 15;
const byte pin_RELAY = 33;

byte mac[] = { 0x04, 0xE9, 0xE5, 0x14, 0xDD, 0x62 };
AsyncHTTPRequest request;
char *user_id_url;

// RFID setup
// SoftwareSerial RFID_SERIAL = SoftwareSerial(0, 1);
#define RFID_SERIAL Serial1
const byte RFID_BUFFER_SIZE = 14;  // RFID DATA FRAME FORMAT: 1byte head (value: 2), 10byte data (2byte version + 8byte tag), 2byte checksum, 1byte tail (value: 3)
const byte RFID_DATA_SIZE = 10;    // 10byte data (2byte version + 8byte tag)
const byte DATA_VERSION_SIZE = 2;  // 2byte version (actual meaning of these two bytes may vary)
const byte DATA_TAG_SIZE = 8;      // 8byte tag
const byte CHECKSUM_SIZE = 2;      // 2byte checksum

uint8_t rfid_buffer[RFID_BUFFER_SIZE];  // used to store an incoming data frame
byte rfid_buffer_index = 0;
// end RFID setup

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

// RFID fns
long int hexstr_to_value(char *str, unsigned int length) {
  char *copy = (char *)malloc((sizeof(char) * length) + 1);
  memcpy(copy, str, sizeof(char) * length);
  copy[length] = '\0';
  long int value = strtol(copy, NULL, 16);
  free(copy);
  return value;
}

unsigned int extract_tag() {
  uint8_t msg_head = rfid_buffer[0];
  uint8_t *msg_data = rfid_buffer + 1;
  uint8_t *msg_data_version = msg_data;
  uint8_t *msg_data_tag = msg_data + 2;
  uint8_t *msg_checksum = rfid_buffer + 11;
  uint8_t msg_tail = rfid_buffer[13];

  Serial.write("--------\n");

  Serial.write("Message-Head: ");
  Serial.println(msg_head);

  Serial.write("Message-Data (HEX): \n");
  for (int i = 0; i < DATA_VERSION_SIZE; ++i) {
    Serial.write((char)msg_data_version[i]);
  }
  Serial.write(" (version)\n");
  for (int i = 0; i < DATA_TAG_SIZE; ++i) {
    Serial.write((char)msg_data_tag[i]);
  }
  Serial.write(" (tag)\n");

  Serial.write("Message-Checksum (HEX): ");
  for (int i = 0; i < CHECKSUM_SIZE; ++i) {
    Serial.write((char)msg_checksum[i]);
  }
  Serial.write("\n");

  Serial.write("Message-Tail: ");
  Serial.println(msg_tail);

  Serial.write("--\n");

  long int tag = hexstr_to_value((char *)msg_data_tag, DATA_TAG_SIZE);
  Serial.write("Extracted Tag: ");
  Serial.println(tag);

  long int checksum = 0;
  for (int i = 0; i < RFID_DATA_SIZE; i += CHECKSUM_SIZE) {
    long int val = hexstr_to_value((char *)(msg_data + i), CHECKSUM_SIZE);
    checksum ^= val;
  }
  Serial.write("Extracted Checksum (HEX): ");
  Serial.print(checksum, HEX);
  if (checksum == hexstr_to_value((char *)msg_checksum, CHECKSUM_SIZE)) {
    Serial.write(" (OK)");
  } else {
    Serial.write(" (NOT OK)");
  }

  Serial.write("\n--------\n");

  return (unsigned int)tag;
}
void readRFID() {
  if (RFID_SERIAL.available() > 0) {
    bool call_extract_tag = false;

    int rfid_val = RFID_SERIAL.read();  // read
    if (rfid_val == -1) {               // no data was read
      return;
    }

    if (rfid_val == 2) {  // RDM630/RDM6300 found a tag => tag incoming
      rfid_buffer_index = 0;
    } else if (rfid_val == 3) {  // tag has been fully transmitted
      call_extract_tag = true;   // extract tag at the end of the function call
    }

    if (rfid_buffer_index >= RFID_BUFFER_SIZE) {  // checking for a buffer overflow (It's very unlikely that a buffer overflow comes up!)
      Serial.println("Error: Buffer overflow detected! ");
      rfid_buffer_index = 0;  // reset buffer index to prevent further issues
      return;
    }

    rfid_buffer[rfid_buffer_index++] = rfid_val;  // everything is alright => copy current value to buffer

    if (call_extract_tag == true) {
      if (rfid_buffer_index == RFID_BUFFER_SIZE) {
        unsigned tag = extract_tag();
        if (tag) {
          Serial.print("Found tag: ");
          Serial.println(tag);
        }
      } else {  // something is wrong... start again looking for preamble (value: 2)
        Serial.println("Error: Incomplete RFID message detected!");
        rfid_buffer_index = 0;  // reset buffer index to prevent further issues
        return;
      }
    }
  }
}


// end RFID fns

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
  RFID_SERIAL.begin(BAUD_RATE);
  
  // todo - load from SD card
  // loadUserURL();

  Serial.println(F("Init complete"));
}

void loop() {
  readRFID();
  delay(100);
}