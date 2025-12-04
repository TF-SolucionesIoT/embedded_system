#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

class Display {
  private:
    Adafruit_SSD1306 oled;
  public:
    Display();
    void init();
    void clear();
    void print(int y, int x, int textSize, String text);
    void display();
    void printPresentation();
};

#endif