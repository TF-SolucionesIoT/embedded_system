#ifndef FALL_DETECTOR_H
#define FALL_DETECTOR_H

class FallDetector {
  private:
    bool fallDetected = false; 
  public:
    FallDetector();
    void begin();
    void on();
    void mpu_read();
    bool wasFallDetected();
};

#endif