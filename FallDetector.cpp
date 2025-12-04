#include <FallDetector.h>
#include <Wire.h>
#include <Arduino.h>
#include <math.h>

const int MPU_addr = 0x68;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
float ax=0, ay=0, az=0, gx=0, gy=0, gz=0;

const float ACC_SENS = 16384.0; 
const float GYRO_SENS = 131.0;  

const float FREEFALL_THRESHOLD = 0.30f;
const float IMPACT_THRESHOLD   = 3.0f;
const unsigned long MAX_FREEFALL_WINDOW = 500;
const unsigned long ORIENTATION_CHECK_DELAY = 200;

enum FDState { IDLE, MAYBE_FREEFALL, MAYBE_IMPACT, CHECK_ORIENTATION };
FDState state = IDLE;
unsigned long freefallStart = 0;
unsigned long impactTime = 0;

float lastStableAx = 0, lastStableAy = 0, lastStableAz = 1;
float postImpactAx = 0, postImpactAy = 0, postImpactAz = 0;

void FallDetector::mpu_read() {
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr, 14, true);
  AcX = Wire.read()<<8 | Wire.read();
  AcY = Wire.read()<<8 | Wire.read();
  AcZ = Wire.read()<<8 | Wire.read();
  Tmp = Wire.read()<<8 | Wire.read();
  GyX = Wire.read()<<8 | Wire.read();
  GyY = Wire.read()<<8 | Wire.read();
  GyZ = Wire.read()<<8 | Wire.read();
}

FallDetector::FallDetector() {
  this->fallDetected = false;
}

void FallDetector::begin() {
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  long axSum=0, aySum=0, azSum=0;
  for(int i=0; i<100; i++) {
    mpu_read();
    axSum += AcX;
    aySum += AcY;
    azSum += AcZ;
    delay(10);
  }
}

static float angleBetween(const float ax1, const float ay1, const float az1,
                          const float ax2, const float ay2, const float az2) {
  float dot = ax1*ax2 + ay1*ay2 + az1*az2;
  float n1 = sqrt(ax1*ax1 + ay1*ay1 + az1*az1);
  float n2 = sqrt(ax2*ax2 + ay2*ay2 + az2*az2);
  if (n1==0 || n2==0) return 0.0f;
  float cosv = dot / (n1*n2);
  if (cosv > 1.0f) cosv = 1.0f;
  if (cosv < -1.0f) cosv = -1.0f;
  return acos(cosv) * 180.0f / PI;
}

void FallDetector::on() {
  mpu_read();

  ax = (float)AcX / ACC_SENS;
  ay = (float)AcY / ACC_SENS;
  az = (float)AcZ / ACC_SENS;
  gx = (float)GyX / GYRO_SENS;
  gy = (float)GyY / GYRO_SENS;
  gz = (float)GyZ / GYRO_SENS;

  float mag = sqrt(ax*ax + ay*ay + az*az);
  unsigned long now = millis();

  if (state == IDLE && mag > 0.85f && mag < 1.15f) {
    lastStableAx = ax;
    lastStableAy = ay;
    lastStableAz = az;
  }

  switch (state) {
    case IDLE:
      if (mag < FREEFALL_THRESHOLD) {
        state = MAYBE_FREEFALL;
        freefallStart = now;
        Serial.println("Freefall detectada");
      }
      break;

    case MAYBE_FREEFALL:
      if (mag > IMPACT_THRESHOLD) {
        state = MAYBE_IMPACT;
        impactTime = now;
        Serial.print("Impacto detectado, mag=");
        Serial.println(mag);
      } else if ((now - freefallStart) > MAX_FREEFALL_WINDOW) {
        Serial.println("Timeout freefall sin impacto");
        state = IDLE;
      }
      break;

    case MAYBE_IMPACT:
      if ((now - impactTime) >= ORIENTATION_CHECK_DELAY) {
        postImpactAx = ax;
        postImpactAy = ay;
        postImpactAz = az;

        float angle = angleBetween(lastStableAx, lastStableAy, lastStableAz,
                                   postImpactAx, postImpactAy, postImpactAz);

        if (angle > 45.0f) {
          fallDetected = true;
        } else {
          Serial.println("No es caída (ángulo pequeño)");
        }
        
        state = IDLE;
      }
      else if (mag > IMPACT_THRESHOLD * 1.5f) {
        Serial.println("Movimiento brusco continuo, no es caída");
        state = IDLE;
      }
      break;

    default:
      state = IDLE;
      break;
  }
}


bool FallDetector::wasFallDetected() {
    bool res = fallDetected;
    fallDetected = false;
    return res;
}