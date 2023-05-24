#include <OneWire.h>
#include <DallasTemperature.h>

const byte pin_TEMP = 7;
const byte pin_FAN = 8;
bool fan_state = false;

OneWire oneWire(pin_TEMP);
DallasTemperature sensors(&oneWire);

float get_temp_C() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}
float get_temp_F() {
  sensors.requestTemperatures();
  return sensors.getTempFByIndex(0);
}

void fan_on() {
  digitalWrite(pin_FAN, HIGH);
  fan_state = true;
}

void fan_off() {
  digitalWrite(pin_FAN, HIGH);
  fan_state = false;
}

void initTemp() {
  sensors.begin();
  pinMode(pin_FAN, OUTPUT);
  digitalWrite(pin_FAN, HIGH);
  delay(250);
  digitalWrite(pin_FAN, LOW);
  if (get_temp_C() == -127.00) {
    Serial.println(F("Temp init failed"));
  } else {
    Serial.println(F("Temp init OK"));
    Serial.print(F("Current C: "));
    Serial.println(get_temp_C());
  }
}
