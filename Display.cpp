#include <Display.h>
#include <Wire.h>
#include <Arduino.h>

#define SCREEN_WIDTH 128  
#define SCREEN_HEIGHT 64  
#define OLED_RESET -1

Display::Display(): oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {

}

void Display::init() {
  if(!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("ERROR: No se encontró la pantalla OLED");
    while (true); 
  }
}

void Display::clear() {
  oled.clearDisplay();
}

void Display::display() {
  oled.display();
}

void Display::print(int y, int x, int textSize, String text) {
  oled.setTextSize(textSize);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(x, y);
  oled.println(text);
}

void Display::printPresentation() {
  oled.setTextSize(3);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("ALERTA");
  oled.setCursor(0, 24);
  oled.println("VITAL");
}
