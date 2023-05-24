#include <TimeOut.h>
const LED_ON = 255;
const LED_OFF = 0;
const unsigned long DEFAULT_TIMEOUT = 2000;

enum ACCESS_LED {
  DATA = 13,
  SUCCESS = 14,
  ERROR = 15,
  NEMO = 16
};

byte last_pin;
bool last_state;

void set_only_led(byte channel, bool state);
void set_access_led(byte channel, bool state);
void set_access_led(ACCESS_LED channel, bool state);
void set_access_led(ACCESS_LED channel, long timeout);
void set_access_led(ACCESS_LED channel);

void initLights() {
  pinMode(pin_DATA, OUTPUT);
  pinMode(pin_SUCCESS, OUTPUT);
  pinMode(pin_ERROR, OUTPUT);
}

void checkLights() {
  analogWrite(pin_DATA, LED_ON);
  delay(250);
  analogWrite(pin_DATA, LED_OFF);
  analogWrite(pin_SUCCESS, LED_ON);
  delay(250);
  analogWrite(pin_SUCCESS, LED_OFF);
  analogWrite(pin_ERROR, LED_ON);
  delay(250);
  analogWrite(pin_ERROR, LED_OFF);
}

void set_only_led(byte pin) {
  for (byte i = static_cast<byte>(DATA); i != static_cast<byte>(NEMO); i++) {
      analogWrite(i, pin == i ? LED_ON : LED_OFF);
    }
  }
}

void set_access_led(byte pin, bool state) {
  if (last_pin == pin && last_state == state) {
    return;
  }
  if (state) {
    set_only_led(pin);
  } else {
    analogWrite(pin, LED_OFF);
  }
  last_pin = pin;
  last_state = state;
}

void set_access_led(ACCESS_LED channel, bool state) {
  set_access_led(static_cast<byte>(channel), state);
}

void set_access_led(ACCESS_LED channel, unsigned long timeout) {
  const byte pin = static_cast<byte>(channel);
  if (DEBUG_ACCESS_LED) {
    Serial.print("setting state of ");
    Serial.print(pin);
    Serial.print(" for (ms) ");
    Serial.println(timeout);
  }
  set_access_led(pin, true);
  TimeOut(timeout, [pin]() {
    if (DEBUG_ACCESS_LED) {
      Serial.print("resetting ");
      Serial.println(pin);
    }
    set_access_led(pin, false);
  });
}

void set_access_led(ACCESS_LED channel) {
  set_access_led(channel, DEFAULT_TIMEOUT);
}