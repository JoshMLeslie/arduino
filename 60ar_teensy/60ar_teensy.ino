#include <Arduino.h>
#include <SPI.h>
#include <ArduinoJson.h>
// Ethernet
#include "QNEthernet.h"
#include <AsyncHTTPRequest_Teensy41.h>
using namespace qindesign::network;
// SD Card
#include <SD.h>

/** FOR TEENSY 4.1 */

/* File must be formatted JSON and alike:
  {
    "ids"?: char[],
    "url"?: char[], // GET "www.foo.com/user/?id=123" => true
    "url_only"?: bool // false
    "relay_timeout"?: unsigned long, // 3600000 ms == 1 hr
    "ids_length"?: unsigned byte // 10
  }
  NB
  - if "url" is not provided, "ids" must be prepopulated
  - "url" should be a simple GET endpoint to validate a single user, returning true or false, whom will be added to the local SD card
  - if "url_only" is true, "url" must be provided. IDs on the SD card will be ignored. SD card IDs will not be updated.
  - ids read from RFID will be left-padded with zeros(0) or right-truncated to match ids_length. There is no partial matching.
*/
const char *filename = "ids.txt";  // 8+3 filename max

const unsigned int BAUD_RATE = 115200;

unsigned long TIMEOUT_RELAY = 3600000;  // 1 hr
const unsigned int TIMEOUT_RFID = 5000;
const unsigned int TIMEOUT_TEMP = 2000;

byte USER_ID_LENGTH = 10;
unsigned int last_tag = 0;

bool relay_live = false;

const byte pin_DATA = 13;
const byte pin_SUCCESS = 14;
const byte pin_ERROR = 15;
const byte pin_RELAY = 33;
const byte pin_BUZZER = 34;

const int MAX_TEMP_C = 30;

volatile unsigned long timeout_check_rfid = 0;
volatile unsigned long timeout_check_relay = 0;
volatile unsigned long timeout_check_temp = 0;

const byte mac[] = { 0x04, 0xE9, 0xE5, 0x14, 0xDD, 0x62 };
bool has_uplink = false;
AsyncHTTPRequest request;
char user_id_url[256];

// debugs
const bool DEBUG_TEMP = false;
const bool DEBUG_RFID = false;
const bool DEBUG_ACCESS_LED = true;

// local files
#include "access_led.h"
#include "rfid.h"
#include "temp.h"
#include "check_id.h"
// funsies
#include "player.h"
#include "song.h"

// SETUP Fns
void initEthernet() {
  Ethernet.setMACAddress(mac);
  Ethernet.begin();
  if (Ethernet.waitForLocalIP(2000)) {
    Serial.print(F("Available at: "));
    Serial.println(Ethernet.localIP());
    has_uplink = true;
  } else {
    Serial.println(F("Uplink failed"));
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
//   sonDocument<64> doc;
//   sonDocument<64> filter;
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

void set_relay_live() {
  digitalWrite(pin_RELAY, HIGH);
  relay_live = true;
  timeout_check_relay = millis();
}
void set_relay_dead() {
  digitalWrite(pin_RELAY, LOW);
  relay_live = false;
  timeout_check_relay = 0;
}

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial);

  // pinMode(pin_BUZZER, OUTPUT);
  // play_song(melody, sizeof(melody)/sizeof(melody[0])/2, 288, pin_BUZZER);

  // Init utilities
  initLights();
  pinMode(pin_RELAY, OUTPUT);
  initEthernet();
  initSD();
  checkSD();
  initRFID();
  initTemp();

  // todo - load from SD card
  // loadUserURL();

  checkLights();
  Serial.println(F("Init complete"));
  // test relay
  // digitalWrite(pin_RELAY, HIGH);
  // delay(5000);
  // digitalWrite(pin_RELAY, LOW);
}

void loop() {
  TimeOut::handler();
  unsigned long now = millis();

  if (now - timeout_check_temp >= TIMEOUT_TEMP ) {
    float temp_c = get_temp_C();
    if (temp_c >= MAX_TEMP_C && !fan_state) {
      fan_on();
    } else if (temp_c < MAX_TEMP_C && fan_state) {
      fan_off();
    }
    timeout_check_temp = now;
  }

  if (now - timeout_check_relay >= TIMEOUT_RELAY) {
    set_access_led(ERROR, true);
    set_relay_dead();
  }

  unsigned int read_tag = readRFID(); // sets pin_DATA HIGH

  if (read_tag && (now - timeout_check_rfid >= TIMEOUT_RFID)) {
    RFID_SERIAL.clear();

    if (
      (last_tag == read_tag && relay_live)
    ) {
      set_access_led(SUCCESS);
      set_relay_dead();
    } else if (checkID_SD(read_tag)) {
      set_access_led(SUCCESS);
      set_relay_live();
      last_tag = read_tag;
    } else {
      if (has_uplink && strlen(user_id_url) > 0) {
        Serial.println(F("Checking for tag at URL"));
        // TODO GET : userid fallback & Update SD

      } else {
        Serial.print(F("Invalid user "));
        Serial.println(read_tag);
        set_access_led(ERROR);
      }
    }
    set_access_led(DATA, false);
    timeout_check_rfid = now;
    Serial.println(F("Finished with tag, resetting rfid."));
  }
}