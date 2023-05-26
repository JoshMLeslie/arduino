/**
RFID reader

Call #readRFID() to get the tag. Uses Teensy Serial1 or SoftwareSerial(6,8) for others
*/
// RFID setup
#if defined(__IMXRT1062__)
  #define RFID_SERIAL Serial1
#else
  #define RFID_SERIAL SoftwareSerial(6, 8)
#endif

const byte RFID_BUFFER_SIZE = 14;  // RFID DATA FRAME FORMAT: 1byte head (value: 2), 10byte data (2byte version + 8byte tag), 2byte checksum, 1byte tail (value: 3)
const byte RFID_DATA_SIZE = 10;    // 10byte data (2byte version + 8byte tag)
const byte DATA_VERSION_SIZE = 2;  // 2byte version (actual meaning of these two bytes may vary)
const byte DATA_TAG_SIZE = 8;      // 8byte tag
const byte CHECKSUM_SIZE = 2;      // 2byte checksum

uint8_t rfid_buffer[RFID_BUFFER_SIZE];  // used to store an incoming data frame
byte rfid_buffer_index = 0;

bool is_reading = false;
// end RFID setup

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

  long int tag = hexstr_to_value((char *)msg_data_tag, DATA_TAG_SIZE);

  if (!DEBUG_RFID) {
    return (unsigned int)tag;
  }

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
unsigned int readRFID() {
  if (RFID_SERIAL.available() > 0) {
    bool call_extract_tag = false;

    if (!is_reading) {
      set_access_led(DATA, true);
      is_reading = true;
    }

    int rfid_val = RFID_SERIAL.read();  // read
    if (rfid_val == -1) {               // no data was read
      is_reading = false;
      return 0;
    }

    if (rfid_val == 2) {  // RDM630/RDM6300 found a tag => tag incoming
      rfid_buffer_index = 0;
    } else if (rfid_val == 3) {  // tag has been fully transmitted
      call_extract_tag = true;   // extract tag at the end of the function call
    }

    if (rfid_buffer_index >= RFID_BUFFER_SIZE) {  // checking for a buffer overflow (It's very unlikely that a buffer overflow comes up!)
      Serial.println("Error: Buffer overflow detected! ");
      rfid_buffer_index = 0;  // reset buffer index to prevent further issues
      is_reading = false;
      return 0;
    }

    rfid_buffer[rfid_buffer_index++] = rfid_val;  // everything is alright => copy current value to buffer

    if (call_extract_tag == true) {
      if (rfid_buffer_index == RFID_BUFFER_SIZE) {
        unsigned int tag = extract_tag();
        if (tag) {
          if (DEBUG_RFID) {
            Serial.print("Found tag: ");
            Serial.println(tag);
          }
          RFID_SERIAL.clear();
          is_reading = false;
          return tag;
        }
      } else {  // something is wrong... start again looking for preamble (value: 2)
        Serial.println("Error: Incomplete RFID message detected!");
        rfid_buffer_index = 0;  // reset buffer index to prevent further issues
        is_reading = false;
        return 0;
      }
    }
  }
  return 0;
}

void initRFID() {
  RFID_SERIAL.begin(9600);
}