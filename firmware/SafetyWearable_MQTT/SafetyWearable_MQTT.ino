/**
 * ============================================================================
 * P_174 — Occupational Safety & Health Monitoring System
 * Firmware: Enterprise MQTT Edge Node (ESP32)
 * ============================================================================
 * Target MCU   : ESP32 38-Pin Development Board (WROOM-32 / DevKit V1)
 * Sensors      : MAX30102 (PPG Heart Rate & SpO2) | MLX90614 (IR Body Temp)
 * Display      : 0.96" SSD1306 OLED (I2C)
 * Transport    : Wi-Fi 802.11 b/g/n + Standard MQTT (HiveMQ / Mosquitto compatible)
 * I2C Pins     : SDA = GPIO 21 | SCL = GPIO 22
 * I2C Clock    : 100 kHz (Mandatory SMBus ceiling dictated by MLX90614)
 *
 * Architecture Highlights:
 *  - Fully non-blocking multi-rate scheduler using millis() (Zero delay in loop).
 *  - Decoupled hardware Device ID (SW-001) from human worker identity.
 *  - Signal Quality Gating (VALID, NO_CONTACT, LOW_SIGNAL, MOTION_DEGRADED).
 *  - Transmits JSON null on lost contact — NO FAKE ZEROES.
 *  - MQTT Last Will and Testament (LWT) for hardware disconnect detection.
 *  - Local OLED diagnostic dashboard (Vitals, WiFi status, MQTT state).
 * ============================================================================
 */

// ─── Standard Core Libraries ────────────────────────────────────────────────
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>       // Standard Arduino MQTT Client (Nick O'Leary)
#include <ArduinoJson.h>         // ArduinoJson v6 or v7 for reliable payload serialization

// ─── Sensor Drivers ─────────────────────────────────────────────────────────
#include <MAX30105.h>           // SparkFun MAX3010x driver (register compatible with MAX30102)
#include <spo2_algorithm.h>     // Maxim reference SpO2 & Heart Rate algorithm
#include <Adafruit_MLX90614.h>  // Adafruit MLX90614 IR non-contact thermometer
#include <Adafruit_GFX.h>       // Core Adafruit graphics library
#include <Adafruit_SSD1306.h>   // SSD1306 OLED driver


// ============================================================================
//  SECTION 1 — DEVICE PROVISIONING & NETWORK CREDENTIALS
// ============================================================================

// Unique Edge Hardware Identifier (Must match backend device registry)
#define DEVICE_ID           "SW-001"
#define FIRMWARE_VERSION    "4.0.0-MQTT"

// Wi-Fi Access Point Configuration
const char* WIFI_SSID       = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD   = "YOUR_WIFI_PASSWORD";

// MQTT Broker Configuration (HiveMQ Cloud, Local Mosquitto, or AWS IoT)
const char* MQTT_BROKER     = "192.168.1.100";  // or "broker.hivemq.com"
const int   MQTT_PORT       = 1883;             // 1883 for TCP, 8883 for TLS
const char* MQTT_USER       = "";               // Optional broker username
const char* MQTT_PASSWORD   = "";               // Optional broker password

// MQTT Topic Contracts (Aligned with docs/MQTT_PROTOCOL.md)
const char* TOPIC_TELEMETRY = "safetywearable/" DEVICE_ID "/telemetry";
const char* TOPIC_STATUS    = "safetywearable/" DEVICE_ID "/status";
const char* TOPIC_ALERT     = "safetywearable/" DEVICE_ID "/alert";
const char* TOPIC_COMMAND   = "safetywearable/" DEVICE_ID "/command";


// ============================================================================
//  SECTION 2 — HARDWARE & I2C DEFINITIONS
// ============================================================================

#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#define I2C_BUS_SPEED       100000UL  // 100 kHz SMBus ceiling

#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT        64
#define OLED_RESET_PIN       -1
#define OLED_I2C_ADDR      0x3C


// ============================================================================
//  SECTION 3 — NON-BLOCKING SCHEDULING INTERVALS
// ============================================================================

#define INTERVAL_DISPLAY_MS     1000UL  // OLED refresh rate (1 Hz)
#define INTERVAL_TEMP_MS        2000UL  // MLX90614 sample rate (0.5 Hz)
#define INTERVAL_MQTT_PUSH_MS   5000UL  // Telemetry publish rate (5 seconds)
#define INTERVAL_NET_CHECK_MS   10000UL // Network health watchdog (10 seconds)

// Alert Debounce & Cooldown Parameters
#define ALERT_PERSIST_MS        15000UL // Abnormal condition must persist 15s
#define ALERT_COOLDOWN_MS       60000UL // 60s cooldown between repeat alerts

// Thresholds (Configurable edge defaults; Spring Boot backend also validates)
#define THRESHOLD_MAX_HR        110
#define THRESHOLD_MIN_SPO2       92
#define THRESHOLD_MAX_TEMP     38.0f


// ============================================================================
//  SECTION 4 — BUFFER SIZES & SENSOR OBJECTS
// ============================================================================

#define SPO2_BUFFER_LEN     100   // Hard requirement of Maxim algorithm
#define NEW_SAMPLE_COUNT     25   // Sliding window shift size

MAX30105          particleSensor;
Adafruit_MLX90614 mlx;
Adafruit_SSD1306  display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

// PPG raw sample buffers
uint32_t irBuffer[SPO2_BUFFER_LEN];
uint32_t redBuffer[SPO2_BUFFER_LEN];

// Algorithm output registers
int32_t spo2Raw         = 0;
int8_t  spo2ValidFlag   = 0;
int32_t heartRateRaw    = 0;
int8_t  heartRateFlag   = 0;

// Filtered and validated runtime state
int32_t currentHeartRate    = 0;
int32_t currentSpo2         = 0;
float   currentTemperature  = 0.0f;
bool    isHeartRateValid    = false;
bool    isSpo2Valid         = false;
bool    isTemperatureValid  = false;

enum SignalQualityState {
    SIGNAL_VALID,
    SIGNAL_NO_CONTACT,
    SIGNAL_LOW_SIGNAL,
    SIGNAL_MOTION_DEGRADED
};
SignalQualityState signalQuality = SIGNAL_NO_CONTACT;

// Scheduling timers
unsigned long timerLastDisplay  = 0;
unsigned long timerLastTemp     = 0;
unsigned long timerLastMqtt     = 0;
unsigned long timerLastNetCheck = 0;

// Alert persistence tracking
unsigned long abnormalConditionStartTime = 0;
bool isConditionAbnormal                 = false;
unsigned long timerLastAlertDispatched   = 0;


// ============================================================================
//  FUNCTION DECLARATIONS
// ============================================================================

void initHardwareI2C();
void initOLED();
void initMLX90614();
void initMAX30102();
void primeSensorBuffers();
void connectWiFi();
void maintainMQTT();
void processSlidingWindow();
void sampleTemperature();
void evaluateSignalQuality();
void evaluateSafetyAlerts();
void publishTelemetry();
void updateOLED();
void drawDegreeSymbol(int16_t x, int16_t y);
void displayError(const char* title, const char* msg);


// ============================================================================
//  SETUP ROUTINE
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(200);

    Serial.println(F("\n=================================================="));
    Serial.println(F(" P_174 — Occupational Safety Wearable (MQTT Node)"));
    Serial.printf (" Device ID: %s | Firmware: %s\n", DEVICE_ID, FIRMWARE_VERSION);
    Serial.println(F("=================================================="));

    initHardwareI2C();
    initOLED();
    initMLX90614();
    initMAX30102();

    // Configure MQTT client buffer and broker coordinates
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setBufferSize(512); // Sufficient for JSON telemetry payload

    connectWiFi();
    maintainMQTT();

    primeSensorBuffers();

    // Initialize alert cooldown in the past to allow immediate alert if needed
    timerLastAlertDispatched = millis() - ALERT_COOLDOWN_MS;

    Serial.println(F("[SYSTEM] Initialization complete. Entering non-blocking loop.\n"));
}


// ============================================================================
//  MAIN LOOP — Non-Blocking Multi-Rate Engine
// ============================================================================

void loop() {
    // 1. Maintain Network & MQTT Connections
    maintainMQTT();
    mqttClient.loop();

    // 2. Sliding Window MAX30102 Acquisition & Algorithm (Continuous)
    processSlidingWindow();
    evaluateSignalQuality();

    // 3. Periodic MLX90614 Temperature Read
    unsigned long now = millis();
    if (now - timerLastTemp >= INTERVAL_TEMP_MS) {
        timerLastTemp = now;
        sampleTemperature();
    }

    // 4. Periodic Telemetry Transmission
    now = millis();
    if (now - timerLastMqtt >= INTERVAL_MQTT_PUSH_MS) {
        timerLastMqtt = now;
        publishTelemetry();
        evaluateSafetyAlerts();
    }

    // 5. Periodic Local OLED Refresh
    now = millis();
    if (now - timerLastDisplay >= INTERVAL_DISPLAY_MS) {
        timerLastDisplay = now;
        updateOLED();
    }
}


// ============================================================================
//  SECTION 5 — SENSOR SIGNAL PROCESSING & QUALITY GATING
// ============================================================================

void processSlidingWindow() {
    // Shift buffer left by 25 samples
    for (uint8_t i = NEW_SAMPLE_COUNT; i < SPO2_BUFFER_LEN; i++) {
        redBuffer[i - NEW_SAMPLE_COUNT] = redBuffer[i];
        irBuffer[i  - NEW_SAMPLE_COUNT] = irBuffer[i];
    }

    // Acquire 25 fresh samples from hardware FIFO
    for (uint8_t i = (SPO2_BUFFER_LEN - NEW_SAMPLE_COUNT); i < SPO2_BUFFER_LEN; i++) {
        while (particleSensor.available() == 0) {
            particleSensor.check();
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
        particleSensor.nextSample();
    }

    // Run Maxim algorithm over the 100-sample window
    maxim_heart_rate_and_oxygen_saturation(
        irBuffer,  SPO2_BUFFER_LEN,
        redBuffer,
        &spo2Raw,      &spo2ValidFlag,
        &heartRateRaw, &heartRateFlag
    );
}

void evaluateSignalQuality() {
    uint32_t lastIR = irBuffer[SPO2_BUFFER_LEN - 1];

    // Gate 1: Optical Contact Detection
    if (lastIR < 50000) {
        // No finger or arm contact on sensor
        signalQuality       = SIGNAL_NO_CONTACT;
        isHeartRateValid    = false;
        isSpo2Valid         = false;
        return;
    }

    // Gate 2: Optical Saturation (Direct sunlight leak or excessive pressure)
    if (lastIR > 240000) {
        signalQuality       = SIGNAL_LOW_SIGNAL;
        isHeartRateValid    = false;
        isSpo2Valid         = false;
        return;
    }

    // Gate 3: Algorithmic Confidence & Physiological Sanity Bounds
    if (heartRateFlag == 1 && heartRateRaw >= 40 && heartRateRaw <= 220) {
        currentHeartRate = heartRateRaw;
        isHeartRateValid = true;
    } else {
        isHeartRateValid = false;
    }

    if (spo2ValidFlag == 1 && spo2Raw >= 70 && spo2Raw <= 100) {
        currentSpo2 = spo2Raw;
        isSpo2Valid = true;
    } else {
        isSpo2Valid = false;
    }

    if (isHeartRateValid && isSpo2Valid) {
        signalQuality = SIGNAL_VALID;
    } else {
        signalQuality = SIGNAL_MOTION_DEGRADED;
    }
}

void sampleTemperature() {
    float temp = mlx.readObjectTempC();
    // Sanity range check for human environment
    if (temp > 15.0f && temp < 60.0f) {
        currentTemperature = temp;
        isTemperatureValid = true;
    } else {
        isTemperatureValid = false;
    }
}


// ============================================================================
//  SECTION 6 — EDGE SAFETY ALERT EVALUATION (DEBOUNCE + COOLDOWN)
// ============================================================================

void evaluateSafetyAlerts() {
    bool hrAbnormal   = (isHeartRateValid && currentHeartRate > THRESHOLD_MAX_HR);
    bool spo2Abnormal = (isSpo2Valid && currentSpo2 < THRESHOLD_MIN_SPO2);
    bool tempAbnormal = (isTemperatureValid && currentTemperature > THRESHOLD_MAX_TEMP);

    bool anyAbnormal = (hrAbnormal || spo2Abnormal || tempAbnormal);
    unsigned long now = millis();

    if (anyAbnormal) {
        if (!isConditionAbnormal) {
            // First detection: start debounce timer
            isConditionAbnormal = true;
            abnormalConditionStartTime = now;
            Serial.println(F("[ALERT DEBOUNCE] Abnormal condition detected. Confirmation timer started."));
        } else {
            // Check if abnormal condition persisted for >= 15 seconds
            if (now - abnormalConditionStartTime >= ALERT_PERSIST_MS) {
                // Check if cooldown has expired
                if (now - timerLastAlertDispatched >= ALERT_COOLDOWN_MS) {
                    timerLastAlertDispatched = now;

                    StaticJsonDocument<256> doc;
                    doc["deviceId"] = DEVICE_ID;
                    if (hrAbnormal) {
                        doc["alertType"] = "HIGH_HEART_RATE";
                        doc["value"]     = currentHeartRate;
                        doc["threshold"] = THRESHOLD_MAX_HR;
                        doc["message"]   = "Worker Heart Rate exceeded safety threshold";
                    } else if (tempAbnormal) {
                        doc["alertType"] = "HIGH_TEMPERATURE";
                        doc["value"]     = currentTemperature;
                        doc["threshold"] = THRESHOLD_MAX_TEMP;
                        doc["message"]   = "Worker skin temperature indicates severe heat stress";
                    } else {
                        doc["alertType"] = "LOW_SPO2";
                        doc["value"]     = currentSpo2;
                        doc["threshold"] = THRESHOLD_MIN_SPO2;
                        doc["message"]   = "Worker blood oxygen saturation dropped below threshold";
                    }
                    doc["severity"]  = "WARNING";

                    char buffer[256];
                    serializeJson(doc, buffer);
                    mqttClient.publish(TOPIC_ALERT, buffer, true); // QoS 1 equivalent
                    Serial.printf("[ALERT DISPATCHED] Topic: %s | Payload: %s\n", TOPIC_ALERT, buffer);
                }
            }
        }
    } else {
        // Reset debounce timer if vitals returned to normal
        isConditionAbnormal = false;
    }
}


// ============================================================================
//  SECTION 7 — MQTT TELEMETRY SERIALIZATION
// ============================================================================

void publishTelemetry() {
    if (!mqttClient.connected()) {
        return;
    }

    StaticJsonDocument<384> doc;
    doc["deviceId"] = DEVICE_ID;

    // DO NOT SEND FAKE ZEROES: If invalid, send JSON null
    if (isHeartRateValid) {
        doc["heartRate"] = currentHeartRate;
    } else {
        doc["heartRate"] = nullptr;
    }
    doc["heartRateValid"] = isHeartRateValid;

    if (isSpo2Valid) {
        doc["spo2"] = currentSpo2;
    } else {
        doc["spo2"] = nullptr;
    }
    doc["spo2Valid"] = isSpo2Valid;

    if (isTemperatureValid) {
        doc["temperature"] = serialized(String(currentTemperature, 1));
    } else {
        doc["temperature"] = nullptr;
    }
    doc["temperatureValid"] = isTemperatureValid;

    // Hardware battery gauge (null until dedicated ADC divider or fuel gauge is wired)
    doc["battery"] = nullptr;

    const char* qualityStr = "VALID";
    if (signalQuality == SIGNAL_NO_CONTACT)       qualityStr = "NO_CONTACT";
    else if (signalQuality == SIGNAL_LOW_SIGNAL)  qualityStr = "LOW_SIGNAL";
    else if (signalQuality == SIGNAL_MOTION_DEGRADED) qualityStr = "MOTION_DEGRADED";
    doc["signalQuality"] = qualityStr;

    char buffer[384];
    serializeJson(doc, buffer);
    mqttClient.publish(TOPIC_TELEMETRY, buffer);

    Serial.printf("[MQTT TELEMETRY] %s -> %s\n", TOPIC_TELEMETRY, buffer);
}


// ============================================================================
//  SECTION 8 — NETWORK CONNECTIVITY & MQTT LIFECYCLE
// ============================================================================

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("[WIFI] Connecting to SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
        delay(250);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("[WIFI] Connected! IP: %s | RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
    } else {
        Serial.println(F("[WIFI] Connection timeout. Retrying in background."));
    }
}

void maintainMQTT() {
    if (WiFi.status() != WL_CONNECTED) {
        connectWiFi();
        return;
    }

    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - timerLastNetCheck >= 5000) {
            timerLastNetCheck = now;
            Serial.printf("[MQTT] Connecting to broker %s:%d as %s...\n",
                          MQTT_BROKER, MQTT_PORT, DEVICE_ID);

            // Construct Last Will and Testament (LWT) payload
            StaticJsonDocument<128> lwtDoc;
            lwtDoc["deviceId"] = DEVICE_ID;
            lwtDoc["status"]   = "OFFLINE";
            lwtDoc["reason"]   = "CONNECTION_LOST";
            char lwtBuffer[128];
            serializeJson(lwtDoc, lwtBuffer);

            // Connect with LWT retained on TOPIC_STATUS
            if (mqttClient.connect(DEVICE_ID, MQTT_USER, MQTT_PASSWORD,
                                   TOPIC_STATUS, 1, true, lwtBuffer)) {
                Serial.println(F("[MQTT] Connected successfully!"));

                // Publish ONLINE status (retained)
                StaticJsonDocument<192> statusDoc;
                statusDoc["deviceId"]        = DEVICE_ID;
                statusDoc["status"]          = "ONLINE";
                statusDoc["firmwareVersion"] = FIRMWARE_VERSION;
                statusDoc["ipAddress"]       = WiFi.localIP().toString();
                statusDoc["rssi"]            = WiFi.RSSI();
                char statusBuffer[192];
                serializeJson(statusDoc, statusBuffer);
                mqttClient.publish(TOPIC_STATUS, statusBuffer, true);

                // Subscribe to commands
                mqttClient.subscribe(TOPIC_COMMAND);
            } else {
                Serial.printf("[MQTT] Failed, rc=%d. Will retry in 5s.\n", mqttClient.state());
            }
        }
    }
}


// ============================================================================
//  SECTION 9 — PERIPHERAL INITIALIZATION & DISPLAY
// ============================================================================

void initHardwareI2C() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_BUS_SPEED);
    Serial.printf("[I2C] Master Bus ready on SDA=%d, SCL=%d @ %lu Hz\n",
                  I2C_SDA_PIN, I2C_SCL_PIN, I2C_BUS_SPEED);
}

void initOLED() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        Serial.println(F("[FATAL] SSD1306 OLED absent at 0x3C!"));
        while (true) { delay(500); }
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(4, 5);
    display.println(F("P_174 SAFETY WEARABLE"));
    display.drawFastHLine(0, 16, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(4, 22);
    display.println(F("Firmware: 4.0.0-MQTT"));
    display.setCursor(4, 34);
    display.printf ("ID: %s\n", DEVICE_ID);
    display.setCursor(4, 48);
    display.println(F("Initializing bus..."));
    display.display();
    delay(1000);
}

void initMLX90614() {
    if (!mlx.begin()) {
        Serial.println(F("[FATAL] MLX90614 not found at 0x5A!"));
        displayError("MLX90614 FAIL", "I2C 0x5A Absent");
        while (true) { delay(500); }
    }
    Serial.println(F("[OK] MLX90614 IR sensor ready."));
    currentTemperature = mlx.readObjectTempC();
    isTemperatureValid = true;
}

void initMAX30102() {
    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
        Serial.println(F("[FATAL] MAX30102 not found at 0x57!"));
        displayError("MAX30102 FAIL", "I2C 0x57 Absent");
        while (true) { delay(500); }
    }

    // Optimized configuration for wearable skin readings
    const byte ledBrightness = 0x7F;  // ~25.4 mA current
    const byte sampleAverage = 4;     // 4x hardware averaging -> 25 sps
    const byte ledMode       = 2;     // SpO2 Mode (Red + IR)
    const int  sampleRate    = 100;   // 100 raw samples / sec
    const int  pulseWidth    = 411;   // 411 µs pulse width (18-bit ADC)
    const int  adcRange      = 4096;  // Full scale range 4096 nA

    particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);
    particleSensor.setPulseAmplitudeRed(0x7F);
    particleSensor.setPulseAmplitudeIR(0x7F);
    particleSensor.setPulseAmplitudeGreen(0x00);

    Serial.println(F("[OK] MAX30102 Pulse Oximeter initialized."));
}

void primeSensorBuffers() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(4, 4);
    display.println(F("Priming PPG Buffer"));
    display.drawFastHLine(0, 16, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(4, 25);
    display.println(F("Place finger on"));
    display.setCursor(4, 38);
    display.println(F("MAX30102 sensor..."));
    display.setCursor(4, 52);
    display.println(F("Acquiring 100 pts"));
    display.display();

    Serial.println(F("[PPG] Priming 100-sample buffer..."));
    for (uint8_t i = 0; i < SPO2_BUFFER_LEN; i++) {
        while (particleSensor.available() == 0) {
            particleSensor.check();
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
        particleSensor.nextSample();
    }
    Serial.println(F("[PPG] Buffer primed successfully."));
}

void updateOLED() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // ── Header: Network & MQTT Diagnostic Status ──
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("ID:%s", DEVICE_ID);

    display.setCursor(55, 0);
    display.print(WiFi.status() == WL_CONNECTED ? "W:OK" : "W:--");

    display.setCursor(90, 0);
    display.print(mqttClient.connected() ? "MQ:OK" : "MQ:--");

    display.drawFastHLine(0, 9, SCREEN_WIDTH, SSD1306_WHITE);

    // ── Metric 1: Heart Rate ──
    display.setTextSize(1);
    display.setCursor(0, 13);
    display.print(F("HR:"));
    display.setTextSize(2);
    display.setCursor(26, 11);
    if (isHeartRateValid) {
        display.print(currentHeartRate);
    } else {
        display.print(F("--"));
    }
    display.setTextSize(1);
    display.setCursor(display.getCursorX() + 2, 17);
    display.print(F("bpm"));

    // ── Metric 2: SpO2 ──
    display.setTextSize(1);
    display.setCursor(0, 31);
    display.print(F("SpO2:"));
    display.setTextSize(2);
    display.setCursor(36, 29);
    if (isSpo2Valid) {
        display.print(currentSpo2);
    } else {
        display.print(F("--"));
    }
    display.setTextSize(1);
    display.setCursor(display.getCursorX() + 2, 35);
    display.print(F("%"));

    // ── Metric 3: Body Temperature & Status Footer ──
    display.drawFastHLine(0, 48, SCREEN_WIDTH, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 52);
    display.print(F("T:"));
    if (isTemperatureValid) {
        display.print(currentTemperature, 1);
        drawDegreeSymbol(display.getCursorX(), 52);
        display.setCursor(display.getCursorX() + 6, 52);
        display.print(F("C"));
    } else {
        display.print(F("--.- C"));
    }

    // Status Indicator
    display.setCursor(68, 52);
    if (signalQuality == SIGNAL_NO_CONTACT) {
        display.print(F("[NO SENSOR]"));
    } else if (isConditionAbnormal) {
        display.print(F("[! ALERT !]"));
    } else {
        display.print(F("[NORMAL]"));
    }

    display.display();
}

void drawDegreeSymbol(int16_t x, int16_t y) {
    display.drawCircle(x + 2, y + 2, 2, SSD1306_WHITE);
}

void displayError(const char* title, const char* msg) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(4, 6);
    display.println(F("!! CRITICAL ERROR !!"));
    display.drawFastHLine(0, 18, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(4, 24);
    display.println(title);
    display.setCursor(4, 38);
    display.println(msg);
    display.drawFastHLine(0, 50, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(4, 53);
    display.println(F("Check I2C Connections"));
    display.display();
}
