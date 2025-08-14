#include <WiFi.h>
#include <Wire.h>
#include <math.h>
#include <HTTPClient.h>

#define ADXL345_ADDR 0x53
#define ADXL345_REG_DEVID 0x00
#define ADXL345_REG_POWER_CTL 0x2D
#define ADXL345_REG_DATA_FORMAT 0x31
#define ADXL345_REG_DATAX0 0x32
#define ADXL345_REG_BW_RATE 0x2C
#define ADXL345_SCALE_FACTOR 0.004

const char* ssid = "MECAP";
const char* password = "8b140b20e7";
const char* serverUrl = "http://192.168.8.54:5000/cast"; // UPDATE THIS IP!

enum GesturePhase { IDLE, UP_DETECTED, PAUSE_DETECTED, DOWN_DETECTED };
GesturePhase phase = IDLE;
unsigned long phaseStart = 0;

// Tuned thresholds
#define UP_THRESHOLD    -1.2
#define DOWN_THRESHOLD   1.0
#define PAUSE_TIME      300
#define MAX_PHASE_GAP   1000

void setup() {
  Serial.begin(115200);
  delay(100);
  
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  Wire.begin();
  delay(100);
  
  byte devId = readRegister(ADXL345_REG_DEVID);
  if (devId != 0xE5) {
    Serial.println("❌ ADXL345 not found!");
    while(1);
  }

  writeRegister(ADXL345_REG_POWER_CTL, 0x08);
  writeRegister(ADXL345_REG_DATA_FORMAT, 0x0B);
  writeRegister(ADXL345_REG_BW_RATE, 0x0A);
  
  Serial.println("✨ Ready for spell gesture! Do an UP-PAUSE-DOWN flick!");
}

void loop() {
  float x, y, z;
  readAcceleration(&x, &y, &z);
  unsigned long now = millis();

  // Debug output (100ms interval)
  static unsigned long lastPrint = 0;
  if (now - lastPrint > 100) {
    Serial.printf("Z: %.2f\tPhase: %d\n", z, phase);
    lastPrint = now;
  }

  switch (phase) {
    case IDLE:
      if (z < UP_THRESHOLD) {
        phase = UP_DETECTED;
        phaseStart = now;
        Serial.println("🪄 UP!");
      }
      break;
      
    case UP_DETECTED:
      if (now - phaseStart > PAUSE_TIME && abs(z) < 0.5) {
        phase = PAUSE_DETECTED;
        phaseStart = now;
        Serial.println("⏸ PAUSE");
      } 
      else if (now - phaseStart > MAX_PHASE_GAP) {
        Serial.println("❌ Reset (too slow)");
        phase = IDLE;
      }
      break;
      
    case PAUSE_DETECTED:
      if (z > DOWN_THRESHOLD) {
        Serial.println("💥 DOWN!");
        triggerSpell();
        phase = IDLE;
      } 
      else if (now - phaseStart > MAX_PHASE_GAP) {
        Serial.println("❌ Reset (no flick)");
        phase = IDLE;
      }
      break;
  }
  delay(20);
}

void triggerSpell() {
  Serial.println("✅ SPELL CAST!");
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
     http.begin(serverUrl);  // example: http://192.168.1.100:5000/cast
     int httpCode = http.POST("");  // ✅ Use POST, not GET

    
    if (httpCode > 0) {
      Serial.printf("🌐 Response: %d\n", httpCode);
    } else {
      Serial.printf("❌ HTTP failed: %s\n", http.errorToString(httpCode).c_str());
      Serial.printf("URL: %s\n", serverUrl);
      Serial.printf("WiFi RSSI: %d dBm\n", WiFi.RSSI());
    }
    http.end();
    
    // Cooldown to prevent double triggers
    delay(500);
  } 
  else {
    Serial.println("❌ WiFi disconnected");
    Serial.print("Reconnecting...");
    WiFi.reconnect();
    while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("\n✅ Reconnected!");
  }
}

void readAcceleration(float *x, float *y, float *z) {
  Wire.beginTransmission(ADXL345_ADDR);
  Wire.write(ADXL345_REG_DATAX0);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345_ADDR, 6, true);

  *x = (int16_t)(Wire.read() | (Wire.read() << 8)) * ADXL345_SCALE_FACTOR;
  *y = (int16_t)(Wire.read() | (Wire.read() << 8)) * ADXL345_SCALE_FACTOR;
  *z = (int16_t)(Wire.read() | (Wire.read() << 8)) * ADXL345_SCALE_FACTOR;
}

byte readRegister(byte reg) {
  Wire.beginTransmission(ADXL345_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(ADXL345_ADDR, 1);
  return Wire.read();
}

void writeRegister(byte reg, byte val) {
  Wire.beginTransmission(ADXL345_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}