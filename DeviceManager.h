#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <Pulseoximeter.h>
#include <Display.h>
#include <FallDetector.h>
#include <BloodPressureReader.h>
#include <BPPulseDetector.h>
#include <Led.h>
#include <Buzzer.h>

class DeviceManager {
  private:
    Pulseoximeter pulseoximeter;
    Display display;
    FallDetector fallDetector;
    BloodPressureReader bpReader;
    BPPulseDetector pulseDetector;
    Led led;
    Buzzer buzzer;
    bool alertActive;
  public:
    DeviceManager();
    void init();
    void manage();
    void performCalibration();
    void manualCalibration();
    void classifyBP(float systolic, float diastolic);
    void startCalibration();
    void updateCalibration();
};

#endif