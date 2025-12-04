#include <DeviceManager.h>

DeviceManager deviceManager;

void setup() {
  deviceManager.init();
}

void loop() {
  deviceManager.manage();
}
