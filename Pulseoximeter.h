#ifndef PULSEOXIMETER_H
#define PULSEOXIMETER_H

#include <Arduino.h>
#include "MAX30105.h"
#include "heartRate.h"

class Pulseoximeter {
  private:
    MAX30105 particleSensor;
    bool fingerPreviouslyDetected;
    uint32_t FINGER_THRESHOLD;

    static const byte RATE_SIZE = 4;
    byte rates[RATE_SIZE];
    byte rateSpot;
    long lastBeat;
    float beatsPerMinute;
    int beatAvg;
    long SAMPLE_INTERVAL_MS;   
    long PRINT_INTERVAL_MS; 
    long lastSampleMillis;
    long lastPrintMillis;
    uint32_t lastIRvalue;
    bool didPrint;

    static const byte SPO2_WINDOW = 100;     
    long redBuffer[SPO2_WINDOW];
    long irBuffer[SPO2_WINDOW];
    byte spo2Index;
    bool spo2Ready;
    int spo2Value;
  public:
    Pulseoximeter();
    void begin();
    void on();
    bool isFingerDetected(long lastIRvalue);
    void detectAndSetTransition(bool fingerDetected);
    void processData(bool fingerDetected, long irValue, unsigned long now);
    void setPrintStatus(bool status);
    int getAverageBPM();
    bool getPrintStatus();
    void resetMeasurements();
    int getSpO2();
};

#endif