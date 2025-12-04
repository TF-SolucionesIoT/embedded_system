#include <DeviceManager.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <ArduinoJson.h>

BLEServer *pServer;
BLECharacteristic *vitalsCharacteristic;
BLECharacteristic *fallCharacteristic;

#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define VITALS_CHAR_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define FALL_CHAR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a9"

#define DOUT_PIN 32
#define SCK_PIN 33

#define LED_PIN 14
#define BUZZER_PIN 19

bool isCalibrated = false;
unsigned long lastPrintTime = 0;

enum CalibrationState {
  CAL_IDLE,
  CAL_WAITING_ZERO,
  CAL_TAKING_ZERO_SAMPLES,
  CAL_WAITING_PRESSURE,
  CAL_TAKING_PRESSURE_SAMPLES,
  CAL_COMPLETE
};

CalibrationState calState = CAL_IDLE;
unsigned long calStartTime = 0;
int calSampleCount = 0;

DeviceManager::DeviceManager(): 
  pulseoximeter(), 
  display(), 
  fallDetector(), 
  bpReader(DOUT_PIN, SCK_PIN, 10), 
  pulseDetector(&bpReader, 20),
  led(LED_PIN),
  buzzer(BUZZER_PIN)
{
  alertActive = false;
}

void DeviceManager::init() {
  Serial.begin(115200);
  delay(1000);
  
  BLEDevice::init("IOT-01");
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);

  vitalsCharacteristic = pService->createCharacteristic(
    VITALS_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | 
    BLECharacteristic::PROPERTY_NOTIFY
  );

  fallCharacteristic = pService->createCharacteristic(
    FALL_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | 
    BLECharacteristic::PROPERTY_NOTIFY
  );

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
  Serial.println("BLE listo, esperando conexión...");

  configTime(0, 0, "pool.ntp.org");
  
  pulseoximeter.begin();
  fallDetector.begin();
  bpReader.begin();
  led.begin();
  buzzer.begin();
  startCalibration();
  
  pulseDetector.setThreshold(3.0f);
  pulseDetector.setMinPeakDistance(400);
  
  display.init();
  display.clear();
  display.printPresentation();
  display.display();
  
  Serial.println("Sistema inicializado\n");
}

void DeviceManager::startCalibration() {
  
  calState = CAL_WAITING_ZERO;
  calStartTime = millis();
  calSampleCount = 0;
  isCalibrated = false;
}

void DeviceManager::updateCalibration() {
  unsigned long now = millis();
  
  switch (calState) {
    case CAL_IDLE:
      break;
      
    case CAL_WAITING_ZERO:
      if (now - calStartTime >= 3000) {
        calState = CAL_TAKING_ZERO_SAMPLES;
        calSampleCount = 0;
      }
      break;
      
    case CAL_TAKING_ZERO_SAMPLES:
      if (bpReader.isReady()) {
        bpReader.readRawInstant();
        calSampleCount++;
        
        if (calSampleCount >= 15) {
          bpReader.calibrateZero();
          long zeroRaw = bpReader.getLastRaw();
          
          calState = CAL_WAITING_PRESSURE;
          calStartTime = millis();
          calSampleCount = 0;
        }
      }
      break;
      
    case CAL_WAITING_PRESSURE:
      {
        unsigned long elapsed = now - calStartTime;
        if (elapsed < 8000) {
          // Mostrar countdown cada segundo
          static unsigned long lastCountdown = 0;
          if (now - lastCountdown >= 1000) {
            lastCountdown = now;
            int secondsLeft = 8 - (elapsed / 1000);
          }
        } else {
          calState = CAL_TAKING_PRESSURE_SAMPLES;
          calSampleCount = 0;
        }
      }
      break;
      
    case CAL_TAKING_PRESSURE_SAMPLES:
      if (bpReader.isReady()) {
        bpReader.readRawInstant();
        calSampleCount++;
        
        if (calSampleCount >= 15) {
          float knownPressure = 100.0f;
          bool success = bpReader.calibratePoint(knownPressure);
          
          if (success) {
            long pressureRaw = bpReader.getLastRaw();
            float offset = bpReader.getOffsetCounts();
            float countsPerKPa = bpReader.getCountsPerKPa();
            
            isCalibrated = true;
          } else {
            isCalibrated = false;
          }
          
          calState = CAL_COMPLETE;
        }
      }
      break;
      
    case CAL_COMPLETE:
      calState = CAL_IDLE;
      break;
  }
}

void DeviceManager::manage() {
  if (calState != CAL_IDLE && calState != CAL_COMPLETE) {
    updateCalibration();
    return;
  }
  
  pulseoximeter.on();
  fallDetector.on();
  
  pulseDetector.update();
  buzzer.update();
  
  unsigned long now = millis();
  
  if (now - lastPrintTime >= 100) {
    lastPrintTime = now;
    
    float current = pulseDetector.getCurrentPressure();
    float systolic = pulseDetector.getSystolic();
    float diastolic = pulseDetector.getDiastolic();
    float bpm = pulseoximeter.getAverageBPM();
      
    pulseDetector.clearNewReading();
  }
  
  if (pulseoximeter.getPrintStatus()) {
    Serial.print(" (BPM=");
    Serial.print(pulseoximeter.getAverageBPM());
    Serial.println(")");
    Serial.print(" (SpO2=");
    Serial.print(pulseoximeter.getSpO2());
    Serial.println(")");
    Serial.print(" (BP=");
    Serial.print(String((int)pulseDetector.getSystolic()));
    Serial.print("/");
    Serial.print(String((int)pulseDetector.getDiastolic()));
    Serial.println(")");
    
    display.clear();
    display.print(0, 0, 2, "BPM: " + String(pulseoximeter.getAverageBPM()));
    display.print(17, 0, 2, "SpO2: " + String(pulseoximeter.getSpO2()) + "%");
    
    if (pulseoximeter.getAverageBPM() < 60 || pulseoximeter.getAverageBPM() > 100 || pulseoximeter.getSpO2() < 92) {
      alertActive = true;
    } else {
      alertActive = false;
    }

    if (pulseDetector.getPulseCount() > 0) {
      display.print(34, 0, 2, "BP: " + String((int)pulseDetector.getSystolic()) + 
                               "/" + String((int)pulseDetector.getDiastolic()));
    }
    
    display.display();

    StaticJsonDocument<200> doc;
    doc["device"] = "IOT-01";
    doc["type"] = "vitals";
    doc["bpm"] = String(pulseoximeter.getAverageBPM());
    doc["spo2"] = String(pulseoximeter.getSpO2());
    doc["bpSystolic"] = String((int)pulseDetector.getSystolic());
    doc["bpDiastolic"] = String((int)pulseDetector.getDiastolic());

    String jsonOut;
    serializeJson(doc, jsonOut);
    vitalsCharacteristic->setValue(jsonOut);
    vitalsCharacteristic->notify();

    pulseoximeter.setPrintStatus(false);
  }

  if (fallDetector.wasFallDetected()) {
    Serial.println("*** CAÍDA DETECTADA ***");
    
    alertActive = true;

    StaticJsonDocument<300> doc;
    doc["device"] = "IOT-01";
    doc["type"] = "fall_alert";
    String jsonOut;
    serializeJson(doc, jsonOut);
    fallCharacteristic->setValue(jsonOut);
    fallCharacteristic->notify();
  }

  if (alertActive) {
    led.on();
    static unsigned long lastBeep = 0;
    if (millis() - lastBeep > 500) {
        buzzer.beep(200); 
        lastBeep = millis();
    }
  } else {
    led.off();
  }
}

void DeviceManager::performCalibration() {
  startCalibration();
}

void DeviceManager::manualCalibration() {

  float offsetCounts = 233000.0f;
  float countsPerKPa = 187.5f;
  
  bpReader.setCalibration(offsetCounts, countsPerKPa);
  
  Serial.print("  Offset:      ");
  Serial.println(offsetCounts, 1);
  Serial.print("  Counts/kPa:  ");
  Serial.println(countsPerKPa, 2);
  Serial.println();
  
  isCalibrated = true;
  calState = CAL_IDLE;
}

void DeviceManager::classifyBP(float systolic, float diastolic) {
  Serial.print("   Clasificación: ");
  
  if (systolic < 90 || diastolic < 60) {
    Serial.println("⬇️  HIPOTENSIÓN (Baja)");
  } else if (systolic < 120 && diastolic < 80) {
    Serial.println("NORMAL");
  } else if (systolic < 130 && diastolic < 80) {
    Serial.println("ELEVADA");
  } else if (systolic < 140 || diastolic < 90) {
    Serial.println("HIPERTENSIÓN Etapa 1");
  } else if (systolic < 180 || diastolic < 120) {
    Serial.println("HIPERTENSIÓN Etapa 2");
  } else {
    Serial.println("CRISIS HIPERTENSIVA");
  }
}