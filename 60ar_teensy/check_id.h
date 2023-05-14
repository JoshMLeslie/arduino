/**
  given an id (alphanumeric length <= 10), this will ping the SD Card to check if it exists
*/
bool checkID_SD(unsigned int id) {
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

  StaticJsonDocument<12> doc;

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
  return false;
}