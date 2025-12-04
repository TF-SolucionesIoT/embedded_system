#include "Buzzer.h"

Buzzer::Buzzer(int pin) {
    this->pin = pin;
    this->beeping = false;
}

void Buzzer::begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void Buzzer::on() {
    digitalWrite(pin, HIGH);
}

void Buzzer::off() {
    digitalWrite(pin, LOW);
}

void Buzzer::beep(int ms) {
    on();
    beeping = true;
    startTime = millis();
    duration = ms;
}

void Buzzer::update() {
    if (beeping) {
        if (millis() - startTime >= duration) {
            off();
            beeping = false;
        }
    }
}