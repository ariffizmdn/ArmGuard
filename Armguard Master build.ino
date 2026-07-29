/*
 * armguard_W001_merged.ino
 * ArmGuard Wearable Safety System — Worker W001
 *
 * Combines:
 * 1. NEO-6M GPS          → /workers/W001/location/  + /history/gps
 * 2. MAX30102 Heart Rate → /workers/W001/sensors/   + /history/bpm
 * 3. MPU6050 Fall Detect → /workers/W001/fall       + /workers/W001/fallTimestamp
 *
 * Hardware (XIAO ESP32-C3):
 * MAX30102   SDA → GPIO6   SCL → GPIO7   VIN → 3.3V
 * MPU6050    SDA → GPIO6   SCL → GPIO7   VIN → 3.3V
 * NEO-6M     TX  → GPIO20  RX  → GPIO21  VIN → 3.3V
 *
 * Libraries:
 * - FirebaseClient (Mobizt)
 * - TinyGPSPlus   (Mikal Hart)
 * - SparkFun MAX3010x (search "MAX30105")
 * - Adafruit MPU6050
 * - Adafruit Unified Sensor
 */

//Indoor GPS testing 
 #define FAKE_GPS
#define FAKE_LAT  //lats coords
#define FAKE_LNG  //lang coords

#define ENABLE_DATABASE
#define ENABLE_USER_AUTH

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>
#include "core/Auth/UserAuth.h"
#include <time.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#ifndef FAKE_GPS
  #include <TinyGPSPlus.h>
  #include <HardwareSerial.h>
#endif

#define WIFI_SSID         "Musa"
#define WIFI_PASSWORD     "kingmusa21"
#define API_KEY           "AIzaSyCghIiXTJBovdmhlPNAktm5ufJlZ8zVlAg"
#define DATABASE_URL      "https://smart-wearable-test1-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL        "ariff67@gmail.com"
#define USER_PASSWORD     "Ariff123"
#define WORKER_ID         "W001"

#define NTP_SERVER        "pool.ntp.org"
#define GMT_OFFSET_SEC    28800
#define DST_OFFSET_SEC    0

#define GPS_PUSH_MS           5000UL
#define MOSFET_PIN            4

#ifndef FAKE_GPS
  #define GPS_RX_PIN  20
  #define GPS_TX_PIN  21
  #define GPS_BAUD    9600
#endif

#define IR_CONTACT_THRESHOLD  30000UL
#define RATE_SIZE             15       // collect 15 beats per cycle
#define BEATS_TO_DISCARD      4        // discard first 4 beats
#define BPM_EMERGENCY_LOW     35
#define BPM_EMERGENCY_HIGH    180
#define BPM_PUSH_INTERVAL_MS  30000UL  // push to Firebase every 30 seconds

#define FREEFALL_THRESHOLD    0.4f
#define IMPACT_THRESHOLD      2.5f
#define FALL_WINDOW_MS        1500
#define FALL_DEBOUNCE_MS      3000
#define MPU_POLL_MS           10

UserAuth          user_auth(API_KEY, USER_EMAIL, USER_PASSWORD);
FirebaseApp       app;
WiFiClientSecure  ssl_client;
using AsyncClient = AsyncClientClass;
AsyncClient       aClient(ssl_client);
RealtimeDatabase  Database;

void asyncCB(AsyncResult &aResult) {
  if (aResult.isError())
    Serial.printf("[Firebase Error] code=%d  %s\n",
                  aResult.error().code(),
                  aResult.error().message().c_str());
}

#ifndef FAKE_GPS
  TinyGPSPlus    gps;
  HardwareSerial gpsSerial(1);
#endif

unsigned long lastGpsPush = 0;
bool triggerGpsPush       = true;

MAX30105 particleSensor;

byte    rates[RATE_SIZE];
byte    rateSpot       = 0;
long    lastBeat       = 0;
float   beatsPerMinute = 0;
int     beatAvg        = 0;
bool    fingerOn       = false;
bool    bufferFull     = false;
unsigned long lastBpmPush     = 0;
unsigned long lastSensorCheck = 0;

String classifyBPM(int bpm) {
  if (bpm < 35)   return "critical";
  if (bpm < 50)   return "low";
  if (bpm <= 100) return "normal";
  if (bpm <= 130) return "elevated";
  return "critical";
}

void resetBPM() {
  memset(rates, 0, sizeof(rates));
  rateSpot       = 0;
  lastBeat       = 0;
  beatsPerMinute = 0;
  beatAvg        = 0;
  fingerOn       = false;
  bufferFull     = false;
  Serial.println("[BPM]  Reset.");
}

void checkSensor() {
  Wire.beginTransmission(0x57);
  byte err = Wire.endTransmission();
  if (err != 0) {
    Serial.println("[BPM]  MAX30102 lost — reinitializing...");
    delay(100);
    particleSensor.begin(Wire, I2C_SPEED_STANDARD);
    particleSensor.setup(0x1F, 4, 2, 400, 411, 4096);
    resetBPM();
    Serial.println("[BPM]  MAX30102 restored.");
  }
}

void pushBPMToFirebase(int avgBPM) {
  if (!app.ready()) {
    Serial.println("[BPM]  Firebase not ready — skipping push.");
    return;
  }

  String base      = String("/workers/") + WORKER_ID;
  String status    = classifyBPM(avgBPM);
  bool   emergency = (avgBPM < BPM_EMERGENCY_LOW || avgBPM > BPM_EMERGENCY_HIGH);

  Database.set<int>(aClient,
    (base + "/sensors/heartRate").c_str(), avgBPM, asyncCB, "setHR");
  Database.set<String>(aClient,
    (base + "/sensors/heartRateStatus").c_str(), status, asyncCB, "setHRStatus");
  Database.set<bool>(aClient,
    (base + "/sensors/bpmEmergency").c_str(), emergency, asyncCB, "setBpmEmergency");

  String histJson = String("{\"t\":") + String(millis() / 1000)
                  + ",\"v\":"         + String(avgBPM) + "}";
  Database.push<object_t>(aClient,
    (base + "/history/bpm").c_str(), object_t(histJson), asyncCB, "pushBPM");

  if (emergency)
    Serial.printf("[BPM]  *** EMERGENCY *** %d BPM (%s)\n", avgBPM, status.c_str());
  else
    Serial.printf("[BPM]  -> Pushed %d BPM (%s)\n", avgBPM, status.c_str());
}

Adafruit_MPU6050 mpu;

bool freeFallDetected = false;
bool impactDetected   = false;
bool fallLatched      = false;

unsigned long freeFallTime = 0;
unsigned long lastFallPush = 0;
unsigned long lastMpuRead  = 0;

time_t getUnixTimestamp() {
  time_t now;
  time(&now);
  return now;
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ArmGuard W001 Booting ===");

  pinMode(MOSFET_PIN, OUTPUT);
  digitalWrite(MOSFET_PIN, LOW); // P-ch: LOW = GPS ON at boot

#ifdef FAKE_GPS
  Serial.printf("[GPS]  FAKE mode  lat=%.6f  lng=%.6f\n", (double)FAKE_LAT, (double)FAKE_LNG);
#else
  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("[WiFi] Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.printf("\n[WiFi] Connected  IP=%s\n", WiFi.localIP().toString().c_str());

  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
  Serial.print("[NTP]  Syncing");
  struct tm timeInfo;
  while (!getLocalTime(&timeInfo)) {
    Serial.print(".");
    delay(500);
  }
  Serial.printf("\n[NTP]  Synced  %02d:%02d:%02d\n",
                timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);

  ssl_client.setInsecure();
  initializeApp(aClient, app, getAuth(user_auth), asyncCB, "authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);
  Serial.println("[Firebase] Initialised.");

  Wire.begin(6, 7);
  Wire.setClock(100000);

  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("[BPM]  MAX30102 not found — check wiring!");
    while (1);
  }
  particleSensor.setup(0x1F, 4, 2, 400, 411, 4096);
  Serial.println("[BPM]  MAX30102 ready.");

  if (!mpu.begin()) {
    Serial.println("[FALL] MPU6050 not found — check wiring!");
    while (1);
  }
  Serial.println("[FALL] MPU6050 ready.");
  Serial.println("=== System Ready ===\n");
}

void loop() {

  app.loop();

  // ── GPS ──────────────────────────────────────────────────────────────────
#ifndef FAKE_GPS
  while (gpsSerial.available()) gps.encode(gpsSerial.read());
#endif

  if (app.ready() && triggerGpsPush && (millis() - lastGpsPush >= GPS_PUSH_MS)) {
    lastGpsPush = millis();
    String base = String("/workers/") + WORKER_ID;

#ifdef FAKE_GPS
    double lat = FAKE_LAT, lng = FAKE_LNG;
    bool valid = true;
#else
    double lat = gps.location.lat(), lng = gps.location.lng();
    bool valid = gps.location.isValid() && gps.location.age() < 5000;
#endif

    Database.set<bool>(aClient,
      (base + "/location/valid").c_str(), valid, asyncCB, "setGpsValid");

    if (valid) {
      Database.set<number_t>(aClient,
        (base + "/location/lat").c_str(), number_t(lat, 6), asyncCB, "setLat");
      Database.set<number_t>(aClient,
        (base + "/location/lng").c_str(), number_t(lng, 6), asyncCB, "setLng");

      String histJson = String("{\"t\":") + String(millis() / 1000)
                      + ",\"lat\":" + String(lat, 6)
                      + ",\"lng\":" + String(lng, 6) + "}";
      Database.push<object_t>(aClient,
        (base + "/history/gps").c_str(), object_t(histJson), asyncCB, "pushGPS");

      Serial.printf("[GPS]  -> lat=%.6f  lng=%.6f\n", lat, lng);

      triggerGpsPush = false;
      digitalWrite(MOSFET_PIN, HIGH); // P-ch: HIGH = GPS OFF
      Serial.println("[POWER] GPS Module powered DOWN.");
    } 

    int rssi = WiFi.RSSI();
    Database.set<int>(aClient,
      (base + "/meta/wifiRSSI").c_str(), rssi, asyncCB, "setRSSI");
  }

  // ── MAX30102 ──────────────────────────────────────────────────────────────
  if (millis() - lastSensorCheck > 2000) {
    lastSensorCheck = millis();
    checkSensor();
  }

  long irValue = particleSensor.getIR();

  if (irValue < IR_CONTACT_THRESHOLD) {
    if (fingerOn) {
      Serial.println("[BPM]  Finger removed.");
      resetBPM();
    }
  } else {
    if (!fingerOn) {
      fingerOn = true;
      Serial.println("[BPM]  Finger on — collecting beats...");
    }

    if (checkForBeat(irValue)) {
      long delta     = millis() - lastBeat;
      lastBeat       = millis();
      beatsPerMinute = 60.0f / (delta / 1000.0f);

      if (beatsPerMinute > 20 && beatsPerMinute < 255) {
        rates[rateSpot++] = (byte)beatsPerMinute;
        Serial.printf("[BPM]  Beat %2d/%d  %.1f BPM\n",
                      rateSpot, RATE_SIZE, beatsPerMinute);

        if (rateSpot >= RATE_SIZE) {
          rateSpot   = 0;      // wrap for next cycle
          bufferFull = true;

          // Discard first 4, average remaining 11
          beatAvg = 0;
          for (byte x = BEATS_TO_DISCARD; x < RATE_SIZE; x++)
            beatAvg += rates[x];
          beatAvg /= (RATE_SIZE - BEATS_TO_DISCARD);

          Serial.printf("[BPM]  Cycle done → avg=%d BPM\n", beatAvg);
        }
      }
    }

    // Push every 30 seconds only after at least one full cycle
    if (bufferFull && beatAvg > 0 && (millis() - lastBpmPush >= BPM_PUSH_INTERVAL_MS)) {
      lastBpmPush = millis();
      pushBPMToFirebase(beatAvg);
    }
  }

  // ── MPU6050 ───────────────────────────────────────────────────────────────
  if (millis() - lastMpuRead >= MPU_POLL_MS) {
    lastMpuRead = millis();

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    float ax = a.acceleration.x / 9.81f;
    float ay = a.acceleration.y / 9.81f;
    float az = a.acceleration.z / 9.81f;
    float totalAcc = sqrt(ax*ax + ay*ay + az*az);

    if (totalAcc < FREEFALL_THRESHOLD) {
      freeFallDetected = true;
      freeFallTime     = millis();
    }

    if (freeFallDetected && totalAcc > IMPACT_THRESHOLD) {
      impactDetected = true;
    }

    if (freeFallDetected && impactDetected &&
        (millis() - freeFallTime < FALL_WINDOW_MS)) {

      if (!fallLatched) {
        fallLatched    = true;
        triggerGpsPush = true;
        digitalWrite(MOSFET_PIN, LOW); // P-ch: LOW = GPS ON
        Serial.println("[POWER] GPS Module powered UP.");
        Serial.println("[FALL] *** Fall detected ***");
      }
    }

    if (millis() - freeFallTime > FALL_WINDOW_MS) {
      freeFallDetected = false;
      impactDetected   = false;
    }
  }

  // ── Fall Firebase push ────────────────────────────────────────────────────
  if (fallLatched && (millis() - lastFallPush > FALL_DEBOUNCE_MS)) {
    if (app.ready()) {
      time_t ts = getUnixTimestamp();
      String base = String("/workers/") + WORKER_ID;

      Database.set<int>(aClient,
        (base + "/fall").c_str(), 1, asyncCB, "setFall");
      Database.set<number_t>(aClient,
        (base + "/fallTimestamp").c_str(), number_t((double)ts, 0), asyncCB, "setFallTs");

      Serial.printf("[FALL] -> Pushed  timestamp=%ld\n", (long)ts);
      lastFallPush   = millis();
      fallLatched    = false;
      triggerGpsPush = false;
      digitalWrite(MOSFET_PIN, HIGH); // P-ch: HIGH = GPS OFF
      Serial.println("[POWER] GPS powered DOWN after fall push.");
    } else {
      triggerGpsPush = true;
      digitalWrite(MOSFET_PIN, LOW);
      Serial.println("[FALL] Firebase not ready — retrying, GPS kept ON.");
    }
  }

  delay(20);
}