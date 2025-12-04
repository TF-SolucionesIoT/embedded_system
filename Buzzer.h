#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

class Buzzer {
private:
    int pin;
    bool beeping;
    unsigned long startTime;
    int duration;
public:
    Buzzer(int pin);

    void begin();
    void on();
    void off();
    void beep(int ms);
    void update();
};

#endif
