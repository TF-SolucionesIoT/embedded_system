#include <Led.h>
#include <Arduino.h>

Led::Led(int ledPin) {
  this->ledPin = ledPin;
}

void Led::begin() {
  pinMode(ledPin, OUTPUT);
}

void Led::on() {
  digitalWrite(ledPin, HIGH);
}

void Led::off() {
  digitalWrite(ledPin, LOW);
}

