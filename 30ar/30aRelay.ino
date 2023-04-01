#include <SoftwareSerial.h>
#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoJson.h>
#include <SD.h>

const char successText[] = "Success";
const char failText[] = "Failed";

const byte pin_RELAY = 2;
const byte pin_ERROR = 4;

bool led_error = false;

// file setup
// const char filename[] = "users.txt";
File usersFile;
// String fileBuffer;

// IDs / Storage
int remote_ids[250];
int local_ids[250];

// RFID setup
const byte BUFFER_SIZE = 14;       // RFID DATA FRAME FORMAT: 1byte head (value: 2), 10byte data (2byte version + 8byte tag), 2byte checksum, 1byte tail (value: 3)
const byte DATA_SIZE = 10;         // 10byte data (2byte version + 8byte tag)
const byte DATA_VERSION_SIZE = 2;  // 2byte version (actual meaning of these two bytes may vary)
const byte DATA_TAG_SIZE = 8;      // 8byte tag
const byte CHECKSUM_SIZE = 2;      // 2byte checksum

SoftwareSerial ssrfid = SoftwareSerial(6, 8);

uint8_t buffer[BUFFER_SIZE];  // used to store an incoming data frame
byte BUFFER_INDEX = 0;
// end RFID setup

// Server setup
byte MAC[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x96, 0x64 };

char SERVER[] = "192.168.1.31";  // IP of file
byte SERVER_PORT = 8080;
String FILE_LOCATION = "/";

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);

EthernetClient client;
// end Ethernet setup

// SD fns
void initSD() {
  Serial.print(F("SD Card access... "));
  // use half speed to resist corruption
  if (!SD.begin(10)) {
    Serial.println(failText);
    Serial.println(F("Local DB unavailable."));
    return;
  }
  Serial.println(successText);

  usersFile = SD.open("users.json", FILE_WRITE);
  if (!usersFile) {
    Serial.println(F("users.json DNE"));
  }
}
// end SD fns

// RFID fns
long extract_tag() {
  uint8_t *msg_data = buffer + 1;  // 10 byte => data contains 2byte version + 8byte tag
  uint8_t *msg_data_tag = msg_data + 2;
  uint8_t *msg_checksum = buffer + 11;  // 2 byte

  // print message that was sent from RDM630/RDM6300
  Serial.println(F("---RFID---"));

  long tag = hexstr_to_value(msg_data_tag, DATA_TAG_SIZE);
  Serial.print(F("Extracted Tag: "));
  Serial.println(tag);

  long checksum = 0;
  for (int i = 0; i < DATA_SIZE; i += CHECKSUM_SIZE) {
    long val = hexstr_to_value(msg_data + i, CHECKSUM_SIZE);
    checksum ^= val;
  }
  Serial.print(F("Extracted Checksum (HEX): "));
  Serial.print(checksum, HEX);
  if (checksum == hexstr_to_value(msg_checksum, CHECKSUM_SIZE)) {
    Serial.print(F(" (OK)"));
  } else {
    Serial.print(F(" (NOT OK)"));
  }

  Serial.println(F("---END RFID---"));

  return tag;
}
long hexstr_to_value(char *str, unsigned int length) {  // converts a hexadecimal value (encoded as ASCII string) to a numeric value
  char *copy = malloc((sizeof(char) * length) + 1);
  memcpy(copy, str, sizeof(char) * length);
  copy[length] = '\0';
  // the variable "copy" is a copy of the parameter "str". "copy" has an additional '\0' element to make sure that "str" is null-terminated.
  long value = strtol(copy, NULL, 16);  // strtol converts a null-terminated string to a long value
  free(copy);                           // clean up
  return value;
}
bool checkForRFID() {
  if (ssrfid.available() > 0) {
    bool call_extract_tag = false;

    int ssvalue = ssrfid.read();  // read
    if (ssvalue == -1) {          // no data was read
      return false;
    }

    if (ssvalue == 2) {  // RDM630/RDM6300 found a tag => tag incoming
      BUFFER_INDEX = 0;
    } else if (ssvalue == 3) {  // tag has been fully transmitted
      call_extract_tag = true;  // extract tag at the end of the function call
    }

    if (BUFFER_INDEX >= BUFFER_SIZE) {  // checking for a buffer overflow (It's very unlikely that an buffer overflow comes up!)
      Serial.println("Error: Buffer overflow detected! ");
      led_error = true;
      return;
    }

    buffer[BUFFER_INDEX++] = ssvalue;  // everything is alright => copy current value to buffer

    if (call_extract_tag == true) {
      if (BUFFER_INDEX == BUFFER_SIZE) {
        long tag = extract_tag();
        Serial.print(F("Found: "));
        Serial.println(tag);
        return tag;
      } else {  // something is wrong... start again looking for preamble (value: 2)
        BUFFER_INDEX = 0;
        return false;
      }
    }
  }
  return false;
}
// end RFID fns
// Server fns
void stopClient() {
  led_error = true;
  client.stop();
}
void connect() {
  Serial.print(F("Initializing Ethernet with DHCP... "));
  if (!Ethernet.begin(MAC, 10000)) {
    Serial.println(failText);
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println(F("Ethernet shield not found?!"));
      led_error = true;
      return;
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println(F("Ethernet cable not connected."));
      led_error = true;
      return;
    }
    Serial.print(F("Attempting static IP... "));
    Ethernet.begin(MAC, ip, myDns);
    if (!Ethernet.linkStatus()) {
      Serial.println(failText);
      return;
    }
  }
  Serial.println(successText);
  Serial.print(F("Local IP: "));
  Serial.println(Ethernet.localIP());

  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.print(F("Connecting to server: "));
  Serial.print(SERVER);
  Serial.print("... ");
  if (client.connect(SERVER, SERVER_PORT)) {
    Serial.println(successText);
  } else {
    Serial.println(failText);
    stopClient();
  }
}
int getIDs() {
  if (client.connected()) {
    client.println("GET " + FILE_LOCATION + " HTTP/1.1");
    client.println("Connection: close");
    client.println();
  }

  // Check HTTP status
  char status[32] = { 0 };
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    stopClient();
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) {
    Serial.println(F("Invalid response"));
    stopClient();
    return;
  }
}

void disconnect() {
  Serial.println();
  Serial.println(F("disconnecting."));
  client.stop();
}
int readRemoteIDs() {
  if (client.available()) {
    Serial.println(F("reading data"));
    DynamicJsonDocument doc(350);  // crashes at 5000, overflows at 500
    DeserializationError error = deserializeJson(doc, client);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      led_error = true;
      client.stop();
      return;
    }

    Serial.println("Response:");
    JsonArray userIDs = doc["userIDs"];
    for (int i = 0; i < userIDs.size(); i++) {
      long id = userIDs[i];
      Serial.println(id);
    }
  }
}
// end Server fns

void setup() {
  Serial.begin(9600);

  while (!Serial)
    ;

  // set pins
  pinMode(pin_RELAY, OUTPUT);
  pinMode(pin_ERROR, OUTPUT);
  pinMode(10, OUTPUT);  // for W5500 shield
  delay(1000);
  initSD();

  // RFID setup
  // ssrfid.begin(9600);
  // ssrfid.listen();

  // Server setup
  connect();
  // getIDs();
  // delay(100);
  // // remote_ids = readRemoteIDs();

  // Serial.println(" INIT DONE");
  // // Serial.println(remote_ids);
  // readRemoteIDs();
}

void loop() {
  long tag = checkForRFID();
  if (tag) {
    Serial.print(F("Found tag: "));
    Serial.println(tag);
  }

  while (led_error) {
    digitalWrite(pin_ERROR, HIGH);
  }
}
