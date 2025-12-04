// BloodPressureReader.cpp (Versión corregida - sin autoCalibrate)
#include "BloodPressureReader.h"
#include <math.h>
#include <stdlib.h>

BloodPressureReader::BloodPressureReader(uint8_t doutPin, uint8_t sckPin, uint8_t samples)
  : _doutPin(doutPin), 
    _sckPin(sckPin),
    _samples((samples >= 1) ? samples : 1),
    _buffer(nullptr), 
    _bufIdx(0), 
    _bufCount(0),
    _lastRaw(0), 
    _lastFilteredCounts(0.0f), 
    _lastKPa(0.0f), 
    _lastMmHg(0.0f),
    _offsetCounts(0.0f), 
    _countsPerKPa(0.0f),
    _hasZeroBeenCalibrated(false), 
    _zeroCountsSaved(0.0f)
{
  // Allocate buffer
  _buffer = (float*)malloc(sizeof(float) * _samples);
  if (_buffer) {
    for (uint8_t i = 0; i < _samples; ++i) {
      _buffer[i] = 0.0f;
    }
  } else {
    // Si malloc falla, asegurar consistencia mínima
    _samples = 1;
    _buffer = (float*)malloc(sizeof(float) * _samples);
    if (_buffer) {
      _buffer[0] = 0.0f;
    }
  }
}

BloodPressureReader::~BloodPressureReader() {
  if (_buffer) {
    free(_buffer);
    _buffer = nullptr;
  }
}

void BloodPressureReader::begin() {
  pinMode(_sckPin, OUTPUT);
  pinMode(_doutPin, INPUT);
  digitalWrite(_sckPin, LOW);
  delay(100);
  
  // Default calibration: zeros (must calibrate before accurate readings)
  _offsetCounts = 0.0f;
  _countsPerKPa = 0.0f;
  _bufIdx = 0;
  _bufCount = 0;
  _hasZeroBeenCalibrated = false;
}

bool BloodPressureReader::isReady() const {
  // DOUT goes LOW when data is ready
  return digitalRead(_doutPin) == LOW;
}

long BloodPressureReader::readRawInstant() {
  // Asume que ya comprobaste isReady() antes de llamar a esta función
  unsigned long value = 0;
  
  // Read 24 bits
  for (uint8_t i = 0; i < 24; ++i) {
    digitalWrite(_sckPin, HIGH);
    delayMicroseconds(1);
    value = (value << 1) | (digitalRead(_doutPin) & 1);
    digitalWrite(_sckPin, LOW);
    delayMicroseconds(1);
  }
  
  // Send 3 extra pulses to select channel A / gain 128 (HX711)
  for (uint8_t p = 0; p < 3; ++p) {
    digitalWrite(_sckPin, HIGH);
    delayMicroseconds(1);
    digitalWrite(_sckPin, LOW);
    delayMicroseconds(1);
  }
  
  // Sign extend 24->32 bits
  if (value & 0x800000UL) {
    value |= 0xFF000000UL;
  }
  
  _lastRaw = (long)value;
  return _lastRaw;
}

void BloodPressureReader::pushBuffer(float counts) {
  if (!_buffer) return;
  
  _buffer[_bufIdx] = counts;
  _bufIdx = (_bufIdx + 1) % _samples;
  
  if (_bufCount < _samples) {
    _bufCount++;
  }
}

float BloodPressureReader::averageBuffer() const {
  if (!_buffer || _bufCount == 0) {
    return 0.0f;
  }
  
  float sum = 0.0f;
  for (uint8_t i = 0; i < _bufCount; ++i) {
    sum += _buffer[i];
  }
  
  return sum / _bufCount;
}

bool BloodPressureReader::update() {
  if (!isReady()) {
    return false;   // No listo → no bloquea
  }
  
  // Leer valor RAW
  long raw = readRawInstant();
  float counts = (float)raw;
  
  // Agregar al buffer de promedio móvil
  pushBuffer(counts);
  float avgCounts = averageBuffer();
  _lastFilteredCounts = avgCounts;
  
  // ✅ ELIMINADO autoCalibrate() - sin sentido físico
  
  // Calcular presión solo si ya está calibrado
  if (_countsPerKPa != 0.0f) {
    float kPa = (avgCounts - _offsetCounts) / _countsPerKPa;
    float mmHg = kPa * KPA_TO_MMHG;
    _lastKPa = kPa;
    _lastMmHg = mmHg;
  } else {
    // Si no está calibrado, presión = 0
    _lastKPa = 0.0f;
    _lastMmHg = 0.0f;
  }
  
  return true;
}

void BloodPressureReader::calibrateZero() {
  long sumRaw = 0;
  const int numSamples = 10;
  
  for (int i = 0; i < numSamples; i++) {
    while (!isReady()) {
      delay(1);
    }
    sumRaw += readRawInstant();
    delay(10);
  }
  
  float avgRaw = (float)sumRaw / numSamples;
  
  if (_buffer) {
    for (uint8_t i = 0; i < _samples; ++i) {
      _buffer[i] = avgRaw;
    }
    _bufCount = _samples;
    _lastFilteredCounts = avgRaw;
    _offsetCounts = avgRaw;
    _hasZeroBeenCalibrated = true;
    _zeroCountsSaved = _offsetCounts;
  }
}

bool BloodPressureReader::calibratePoint(float knownMmHg) {
  if (!_hasZeroBeenCalibrated) {
    return false;
  }
  
  long sumRaw = 0;
  const int numSamples = 10;
  
  for (int i = 0; i < numSamples; i++) {
    while (!isReady()) {
      delay(1);
    }
    sumRaw += readRawInstant();
    delay(10);
  }
  
  float avgRaw = (float)sumRaw / numSamples;
  
  if (_buffer) {
    for (uint8_t i = 0; i < _samples; ++i) {
      _buffer[i] = avgRaw;
    }
    _bufCount = _samples;
  }
  
  float currentCounts = avgRaw;
  
  float knownKPa = knownMmHg / KPA_TO_MMHG;
  
  float countsDelta = currentCounts - _zeroCountsSaved;
  
  if (fabs(countsDelta) < 1e-6) {
    return false;
  }
  
  _countsPerKPa = countsDelta / knownKPa;
  
  _offsetCounts = _zeroCountsSaved;
  
  return true;
}

void BloodPressureReader::setCalibration(float offsetCounts, float countsPerKPa) {
  _offsetCounts = offsetCounts;
  _countsPerKPa = countsPerKPa;
  _hasZeroBeenCalibrated = true;
  _zeroCountsSaved = offsetCounts;
}

long BloodPressureReader::getLastRaw() const {
  return _lastRaw;
}

float BloodPressureReader::getLastFilteredCounts() const {
  return _lastFilteredCounts;
}

float BloodPressureReader::getPressureKPa() const {
  return _lastKPa;
}

float BloodPressureReader::getPressureMmHg() const {
  return _lastMmHg;
}

float BloodPressureReader::getOffsetCounts() const {
  return _offsetCounts;
}

float BloodPressureReader::getCountsPerKPa() const {
  return _countsPerKPa;
}

void BloodPressureReader::reset() {
  if (!_buffer) return;
  
  for (uint8_t i = 0; i < _samples; ++i) {
    _buffer[i] = 0.0f;
  }
  
  _bufIdx = 0;
  _bufCount = 0;
  _lastRaw = 0;
  _lastFilteredCounts = 0.0f;
  _lastKPa = 0.0f;
  _lastMmHg = 0.0f;
}