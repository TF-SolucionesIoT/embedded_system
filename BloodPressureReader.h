#ifndef BLOOD_PRESSURE_READER_H
#define BLOOD_PRESSURE_READER_H

#include <Arduino.h>

class BloodPressureReader {
public:
  BloodPressureReader(uint8_t doutPin, uint8_t sckPin, uint8_t samples = 10);
  
  ~BloodPressureReader();

  void begin();

  bool update();

  long readRawInstant();

  void calibrateZero();

  bool calibratePoint(float knownMmHg);

  void setCalibration(float offsetCounts, float countsPerKPa);

  long getLastRaw() const;
  float getLastFilteredCounts() const;
  float getPressureKPa() const;
  float getPressureMmHg() const;

  float getOffsetCounts() const;
  float getCountsPerKPa() const;

  void reset();

  bool isReady() const;

  static constexpr float KPA_TO_MMHG = 7.50062f;

private:
  uint8_t _doutPin;
  uint8_t _sckPin;
  uint8_t _samples;

  float* _buffer;
  uint8_t _bufIdx;
  uint8_t _bufCount;

  long  _lastRaw;
  float _lastFilteredCounts;
  float _lastKPa;
  float _lastMmHg;

  float _offsetCounts;
  float _countsPerKPa;

  bool _hasZeroBeenCalibrated;
  float _zeroCountsSaved;

  void pushBuffer(float counts);
  float averageBuffer() const;
};

#endif