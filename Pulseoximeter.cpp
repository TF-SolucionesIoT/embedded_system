#include <Pulseoximeter.h>
#include <Wire.h>
#include <limits.h>


static int calculateSpO2FromBuffers(long *redBuf, long *irBuf, byte len) {
  if (len == 0) return -1;

  long minR = LONG_MAX, maxR = LONG_MIN;
  long minIR = LONG_MAX, maxIR = LONG_MIN;
  long sumR = 0, sumIR = 0;

  for (byte i = 0; i < len; i++) {
    long r = redBuf[i];
    long ir = irBuf[i];
    sumR += r;
    sumIR += ir;
    if (r < minR) minR = r;
    if (r > maxR) maxR = r;
    if (ir < minIR) minIR = ir;
    if (ir > maxIR) maxIR = ir;
  }

  float dcR = (float)sumR / len;
  float dcIR = (float)sumIR / len;
  float acR = (float)(maxR - minR);
  float acIR = (float)(maxIR - minIR);

  if (dcR <= 0.0f || dcIR <= 0.0f || acIR <= 0.0f) return -1;

  float ratio = (acR / dcR) / (acIR / dcIR);
  float spo2 = 110.0f - 25.0f * ratio;

  if (spo2 > 100.0f) spo2 = 100.0f;
  if (spo2 < 0.0f) spo2 = 0.0f;

  return (int)round(spo2);
}


Pulseoximeter::Pulseoximeter(): particleSensor() {
  this->fingerPreviouslyDetected = false;
  this->FINGER_THRESHOLD = 50000;

  this->rateSpot = 0;
  this->lastBeat = 0;
  this->beatsPerMinute = 0;
  this->beatAvg = 0;
  this->SAMPLE_INTERVAL_MS = 10;   
  this->PRINT_INTERVAL_MS = 5000; 
  this->lastSampleMillis = 0;
  this->lastPrintMillis = 0;
  this->lastIRvalue = 0;
  this->didPrint = false;

  this->spo2Index = 0;
  this->spo2Ready = false;
  this->spo2Value = -1;
  for (byte i = 0; i < SPO2_WINDOW; i++) {
    this->redBuffer[i] = 0;
    this->irBuffer[i] = 0;
  }
}


void Pulseoximeter::begin() {
  Serial.println("Iniciando Pulseoximeter...");
  Wire.begin();

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 no encontrado. Verifica wiring.");
    while (1) delay(1000);
  }

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x1E);
  particleSensor.setPulseAmplitudeGreen(0);

  resetMeasurements();

  this->lastSampleMillis = millis();
  this->lastPrintMillis = millis();
  this->lastBeat = millis();
}

void Pulseoximeter::resetMeasurements() {
  for (byte i = 0; i < RATE_SIZE; i++) this->rates[i] = 0;
  this->rateSpot = 0;
  this->beatAvg = 0;
  this->lastBeat = millis();
}


bool Pulseoximeter::isFingerDetected(long lastIRvalue) {
  if (lastIRvalue >= FINGER_THRESHOLD)  {
    return true;
  }
  return false; 
}

void Pulseoximeter::detectAndSetTransition(bool fingerDetected) {
  if (fingerDetected && !fingerPreviouslyDetected) {
    resetMeasurements();
    Serial.println("Nuevo dedo detectado → reiniciando medición…");
    this->fingerPreviouslyDetected = fingerDetected;
  }
  if (!fingerDetected && fingerPreviouslyDetected) {
    this->fingerPreviouslyDetected = fingerDetected;
  }
}

void Pulseoximeter::processData(bool fingerDetected, long irValue, unsigned long now) {
  if (fingerDetected) {

    if (now - lastSampleMillis >= SAMPLE_INTERVAL_MS) {
      lastSampleMillis += SAMPLE_INTERVAL_MS;

      long redValue = particleSensor.getRed(); 
      irBuffer[spo2Index] = irValue;
      redBuffer[spo2Index] = redValue;
      spo2Index++;
      if (spo2Index >= SPO2_WINDOW) {
        spo2Index = 0;
        spo2Ready = true;
        spo2Value = calculateSpO2FromBuffers(redBuffer, irBuffer, SPO2_WINDOW);
      }

      if (checkForBeat(irValue)) {
        long delta = now - lastBeat;
        lastBeat = now;

        if (delta > 0) {
          float instantBPM = 60.0f / (delta / 1000.0f);

          if (instantBPM > 20 && instantBPM < 250) {
            rates[rateSpot++] = (byte)instantBPM;
            rateSpot %= RATE_SIZE;

            long sum = 0;
            byte cnt = 0;
            for (byte x = 0; x < RATE_SIZE; x++) {
              if (rates[x] > 0) { sum += rates[x]; cnt++; }
            }
            if (cnt > 0) beatAvg = sum / cnt;
          }
        }
      }
    }
  }
  else {
    beatAvg = 0;
    spo2Value = -1;
  }

}


int Pulseoximeter::getAverageBPM() { 
  return beatAvg; 
}


void Pulseoximeter::setPrintStatus(bool status) {
  this->didPrint = status;
}

bool Pulseoximeter::getPrintStatus() {
  return didPrint;
}

int Pulseoximeter::getSpO2() {
  return spo2Value;
}


void Pulseoximeter::on() {

  unsigned long now = millis();
  long irValue = particleSensor.getIR();
  this->lastIRvalue = irValue;

  bool fingerDetected = isFingerDetected(lastIRvalue);

  detectAndSetTransition(fingerDetected);

  processData(fingerDetected, irValue, now);

  if (now - lastPrintMillis >= PRINT_INTERVAL_MS) {
    lastPrintMillis += PRINT_INTERVAL_MS;

    if (fingerDetected) {
      if (beatAvg > 0) {
        // setPrintStatus(true);
        if (spo2Value >= 0) {
          setPrintStatus(true);
          // Serial.print("SpO2: "); Serial.println(spo2Value);
        }
      }
    } else {
      Serial.println("Sin dedo, no se imprime BPM.");
    }

    // setPrintStatus(false);
  }
  
}