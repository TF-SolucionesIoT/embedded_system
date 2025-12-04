#ifndef BP_PULSE_DETECTOR_H
#define BP_PULSE_DETECTOR_H

#include "BloodPressureReader.h"

class BPPulseDetector {
public:
  BPPulseDetector(BloodPressureReader* reader, uint8_t windowSize = 20);
  ~BPPulseDetector();
  
  void update();
  
  float getSystolic() const { return _systolic; }
  float getDiastolic() const { return _diastolic; }
  float getCurrentPressure() const { return _currentPressure; }
  
  bool hasNewReading() const { return _hasNewReading; }
  void clearNewReading() { _hasNewReading = false; }
  
  void setThreshold(float threshold) { _threshold = threshold; }
  void setMinPeakDistance(uint16_t ms) { _minPeakDistance = ms; }
  
  float getBPM() const { return _bpm; }
  uint32_t getPulseCount() const { return _pulseCount; }
  
private:
  BloodPressureReader* _reader;
  
  float* _pressureBuffer;
  uint8_t _bufferSize;
  uint8_t _bufferIndex;
  uint8_t _bufferCount;
  
  float _currentPressure;
  float _systolic;
  float _diastolic;
  
  bool _hasNewReading;
  float _lastPeak;
  float _lastValley;
  unsigned long _lastPeakTime;
  unsigned long _lastValleyTime;
  
  float _threshold;
  uint16_t _minPeakDistance;
  
  float _bpm;
  uint32_t _pulseCount;
  
  enum State {
    STATE_IDLE,
    STATE_PRESSURE_RISING,
    STATE_PRESSURE_FALLING
  };
  State _state;
  
  void pushBuffer(float pressure);
  float getBufferMax() const;
  float getBufferMin() const;
  float getBufferAverage() const;
  bool isLocalMaximum(uint8_t index) const;
  bool isLocalMinimum(uint8_t index) const;
  void detectPulse();
};

#endif