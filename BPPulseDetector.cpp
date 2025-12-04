#include "BPPulseDetector.h"
#include <stdlib.h>
#include <math.h>

BPPulseDetector::BPPulseDetector(BloodPressureReader* reader, uint8_t windowSize)
  : _reader(reader),
    _bufferSize(windowSize),
    _bufferIndex(0),
    _bufferCount(0),
    _currentPressure(0.0f),
    _systolic(0.0f),
    _diastolic(0.0f),
    _hasNewReading(false),
    _lastPeak(0.0f),
    _lastValley(0.0f),
    _lastPeakTime(0),
    _lastValleyTime(0),
    _threshold(5.0f),
    _minPeakDistance(400),
    _bpm(0.0f),
    _pulseCount(0),
    _state(STATE_IDLE)
{
  _pressureBuffer = (float*)malloc(sizeof(float) * _bufferSize);
  if (_pressureBuffer) {
    for (uint8_t i = 0; i < _bufferSize; i++) {
      _pressureBuffer[i] = 0.0f;
    }
  }
}

BPPulseDetector::~BPPulseDetector() {
  if (_pressureBuffer) {
    free(_pressureBuffer);
  }
}

void BPPulseDetector::pushBuffer(float pressure) {
  if (!_pressureBuffer) return;
  
  _pressureBuffer[_bufferIndex] = pressure;
  _bufferIndex = (_bufferIndex + 1) % _bufferSize;
  
  if (_bufferCount < _bufferSize) {
    _bufferCount++;
  }
}

float BPPulseDetector::getBufferMax() const {
  if (!_pressureBuffer || _bufferCount == 0) return 0.0f;
  
  float maxVal = _pressureBuffer[0];
  for (uint8_t i = 1; i < _bufferCount; i++) {
    if (_pressureBuffer[i] > maxVal) {
      maxVal = _pressureBuffer[i];
    }
  }
  return maxVal;
}

float BPPulseDetector::getBufferMin() const {
  if (!_pressureBuffer || _bufferCount == 0) return 0.0f;
  
  float minVal = _pressureBuffer[0];
  for (uint8_t i = 1; i < _bufferCount; i++) {
    if (_pressureBuffer[i] < minVal) {
      minVal = _pressureBuffer[i];
    }
  }
  return minVal;
}

float BPPulseDetector::getBufferAverage() const {
  if (!_pressureBuffer || _bufferCount == 0) return 0.0f;
  
  float sum = 0.0f;
  for (uint8_t i = 0; i < _bufferCount; i++) {
    sum += _pressureBuffer[i];
  }
  return sum / _bufferCount;
}

void BPPulseDetector::detectPulse() {
  if (_bufferCount < _bufferSize) return;
  
  unsigned long now = millis();
  float currentMax = getBufferMax();
  float currentMin = getBufferMin();
  float currentAvg = getBufferAverage();
  
  switch (_state) {
    case STATE_IDLE:
      if (_currentPressure > currentAvg + _threshold) {
        _state = STATE_PRESSURE_RISING;
        _lastValley = currentMin;
        _lastValleyTime = now;
      }
      break;
      
    case STATE_PRESSURE_RISING:
      if (_currentPressure < currentMax - _threshold) {
        if (now - _lastPeakTime >= _minPeakDistance) {
          _lastPeak = currentMax;
          _lastPeakTime = now;
          _state = STATE_PRESSURE_FALLING;
          _pulseCount++;
          
          if (_lastValleyTime > 0) {
            float interval = (now - _lastValleyTime) / 1000.0f;
            if (interval > 0.3f) {
              _bpm = 60.0f / interval;
            }
          }
          
          _systolic = _lastPeak;
        }
        _state = STATE_PRESSURE_FALLING;
      }
      break;
      
    case STATE_PRESSURE_FALLING:
      if (_currentPressure > currentMin + _threshold) {
        _lastValley = currentMin;
        _lastValleyTime = now;
        _state = STATE_PRESSURE_RISING;
        
        _diastolic = _lastValley;
        
        _hasNewReading = true;
      }
      break;
  }
}

void BPPulseDetector::update() {
  if (!_reader) return;
  
  if (_reader->update()) {
    _currentPressure = _reader->getPressureMmHg();
    
    pushBuffer(_currentPressure);
    
    detectPulse();
  }
}