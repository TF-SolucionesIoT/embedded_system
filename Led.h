#ifndef LED_H
#define LED_H

class Led {
  private:
    int ledPin;
  public:
    Led(int ledPin);
    void begin();
    void on();
    void off();
};

#endif