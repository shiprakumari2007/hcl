/**
 * ============================================================================
 * Occupational Safety Wearable — Phase 3 Firmware (IoT & Cloud Alerts)
 * ============================================================================
 * Target MCU  : ESP32 38-pin (ESP32-WROOM-32 / DevKit V1)
 * Sensors     : MAX30102 (HR + SpO2) | MLX90614 (IR Temp)
 * Display     : 0.96" SSD1306 OLED (I2C)
 * IoT Cloud   : Blynk IoT 2.0 (via WiFi)
 * I2C Pins    : SDA = GPIO 21 | SCL = GPIO 22
 * I2C Speed   : 100 kHz (SMBus compatible — limited by MLX90614)
 *
 * ============================================================================
 * BLYNK 2.0 CONFIGURATION (MUST BE AT THE VERY TOP BEFORE ANY INCLUDES)
 * ============================================================================
 * Obtain these credentials from your Blynk.Console dashboard under:
 * Device Info / Template Settings.
 */
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxx"       // Replace with your Template ID
#define BLYNK_TEMPLATE_NAME "Safety Wearable"  // Replace with your Template Name
#define BLYNK_AUTH_TOKEN    "YourAuthToken"    // Replace with your Device Auth Token

// Comment this out to disable verbose Blynk debug prints over Serial:
#define BLYNK_PRINT Serial

// ─── Standard & Platform Libraries ───────────────────────────────────────────
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>               // Arduino I2C master library

// ─── Sensor Libraries ────────────────────────────────────────────────────────
#include <MAX30105.h>           // SparkFun MAX3010x driver (covers MAX30102)
#include <spo2_algorithm.h>     // Maxim reference SpO2 + HR algorithm
#include <Adafruit_MLX90614.h>  // Adafruit MLX90614 IR thermometer driver

// ─── Display Libraries ───────────────────────────────────────────────────────
#include <Adafruit_GFX.h>       // Adafruit graphics primitives base class
#include <Adafruit_SSD1306.h>   // Adafruit SSD1306 OLED driver


// ============================================================================
//  SECTION 1 — WI-FI & BLYNK NETWORK CREDENTIALS
// ============================================================================

char ssid[] = "YOUR_WIFI_SSID";         // Replace with your Wi-Fi network name
char pass[] = "YOUR_WIFI_PASSWORD";     // Replace with your Wi-Fi password


// ============================================================================
//  SECTION 2 — HARDWARE CONSTANTS
// ============================================================================

// I2C Pin Assignment (ESP32 38-pin hardware I2C peripheral)
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22

// OLED Display Geometry & Address
#define SCREEN_WIDTH        128   // SSD1306 horizontal resolution (pixels)
#define SCREEN_HEIGHT        64   // SSD1306 vertical resolution (pixels)
#define OLED_RESET_PIN       -1   // -1 = share the ESP32 EN/RESET line
#define OLED_I2C_ADDR      0x3C  // Most common: 0x3C. If not found, try 0x3D.

// I2C Bus Speed: 100 kHz (Governed by MLX90614 SMBus limit)
#define I2C_BUS_SPEED      100000UL


// ============================================================================
//  SECTION 3 — TIMING CONSTANTS (NON-BLOCKING INTERVALS)
// ============================================================================

// How often the OLED refreshes (ms). 1000ms = 1 Hz.
#define DISPLAY_REFRESH_MS  1000UL

// How often MLX90614 is polled (ms). Reduces I2C traffic during PPG sampling.
#define TEMP_REFRESH_MS     2000UL

// How often sensor data is pushed to Blynk Cloud (ms).
// 5000ms = 5 seconds. Pushing faster than 1s risks rate-limiting/flooding.
#define BLYNK_PUSH_MS       5000UL

// Emergency alert cooldown interval (ms).
// 60,000ms = 60 seconds. Prevents push notification spam if vitals remain critical.
#define ALERT_COOLDOWN_MS   60000UL


// ============================================================================
//  SECTION 4 — SAFETY THRESHOLDS (OCCUPATIONAL RISK DETECTION)
// ============================================================================

#define THRESHOLD_MAX_HR        110     // Tachycardia / heat exhaustion warning (> 110 BPM)
#define THRESHOLD_MIN_SPO2       92     // Hypoxia / respiratory distress warning (< 92%)
#define THRESHOLD_MAX_TEMP     38.0f    // Heat stroke / hyperthermia warning (> 38.0 °C)


// ============================================================================
//  SECTION 5 — BLYNK VIRTUAL PIN ASSIGNMENTS
// ============================================================================

#define VPIN_HEART_RATE     V1    // Integer: BPM
#define VPIN_SPO2           V2    // Integer: %
#define VPIN_BODY_TEMP      V3    // Float: °C
#define VPIN_ALERT_STATUS   V4    // String: Normal or Alert message


// ============================================================================
//  SECTION 6 — MAX30102 SpO2 ALGORITHM CONSTANTS
// ============================================================================

#define SPO2_BUFFER_LEN     100   // Hard requirement of Maxim reference algorithm
#define NEW_SAMPLE_COUNT     25   // Sliding window shift size (~1 sec fresh data)


// ============================================================================
//  SECTION 7 — HARDWARE SENSOR OBJECTS
// ============================================================================

MAX30105          particleSensor;
Adafruit_MLX90614 mlx;
Adafruit_SSD1306  display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);


// ============================================================================
//  SECTION 8 — DATA BUFFERS & RUNTIME STATE
// ============================================================================

uint32_t irBuffer[SPO2_BUFFER_LEN];
uint32_t redBuffer[SPO2_BUFFER_LEN];

int32_t spo2Raw         = 0;
int8_t  spo2ValidFlag   = 0;
int32_t heartRateRaw    = 0;
int8_t  heartRateFlag   = 0;

// Last-known good biometric readings
int32_t displayHR       = 0;
int32_t displaySpO2     = 0;
float   displayTempC    = 0.0f;
bool    hrGoodValue     = false;
bool    spo2GoodValue   = false;

// Non-blocking timer tracking variables
unsigned long lastDisplayRefresh = 0;
unsigned long lastTempRead       = 0;
unsigned long lastBlynkPush      = 0;

/**
 * Cooldown Tracking Logic:
 * lastAlertTrigger stores the millis() timestamp of when the last alert event
 * was dispatched. An alert will only be allowed if:
 * (currentMillis - lastAlertTrigger >= ALERT_COOLDOWN_MS)
 * Setting it initially to -ALERT_COOLDOWN_MS allows an immediate trigger on boot
 * if an emergency occurs right away.
 */
unsigned long lastAlertTrigger   = 0;


// ============================================================================
//  FUNCTION PROTOTYPES
// ============================================================================

void initOLED();
void initMLX90614();
void initMAX30102();
void primeSensorBuffers();
void updateDisplay();
void displayError(const char* title, const char* msg);
void drawDegreeSymbol(int16_t x, int16_t y);
void sendDataToBlynk();
void evaluateSafetyAlerts();


// ============================================================================
//  SETUP — Runs once on boot
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(150);

    Serial.println(F("\n============================================"));
    Serial.println(F("  Occupational Safety Wearable — Phase 3"));
    Serial.println(F("  IoT Integration & Safety Alert Engine"));
    Serial.println(F("============================================"));

    // ── 1. Initialize I2C Bus ────────────────────────────────────────────────
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_BUS_SPEED);
    Serial.printf("[BUS] I2C initialized: SDA=GPIO%d, SCL=GPIO%d, Speed=%lu Hz\n",
                  I2C_SDA_PIN, I2C_SCL_PIN, I2C_BUS_SPEED);

    // ── 2. Initialize Hardware Peripherals ───────────────────────────────────
    initOLED();
    initMLX90614();
    initMAX30102();

    // ── 3. Connect to Wi-Fi and Blynk Cloud ───────────────────────────────────
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(4, 10);
    display.println(F("Connecting to Wi-Fi"));
    display.setCursor(4, 25);
    display.println(F("and Blynk Cloud..."));
    display.drawFastHLine(0, 40, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(4, 48);
    display.println(ssid);
    display.display();

    Serial.print(F("[WIFI] Connecting to "));
    Serial.println(ssid);

    // Initialize Blynk connection. (Timeout configured internally by Blynk)
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    if (Blynk.connected()) {
        Serial.println(F("[BLYNK] Connected successfully to Blynk.Cloud!"));
    } else {
        Serial.println(F("[BLYNK] Warning: Could not connect immediately. Continuing in background."));
    }

    // ── 4. Prime MAX30102 FIFO Buffers ───────────────────────────────────────
    primeSensorBuffers();

    // Allow alert immediately if threshold breached on startup
    lastAlertTrigger = millis() - ALERT_COOLDOWN_MS;

    Serial.println(F("\n[READY] System online. Non-blocking IoT loop starting.\n"));
}


// ============================================================================
//  MAIN LOOP — Non-Blocking Multi-Rate Engine
// ============================================================================

void loop() {
    // ── Maintain Blynk Cloud Connection and Process Inbound Packets ─────────
    // Blynk.run() is lightweight when no new packets are pending.
    // It keeps the connection alive and handles ping responses.
    Blynk.run();

    // ╔════════════════════════════════════════════════════════════════════════╗
    // ║  TASK A — MAX30102 Sliding Window Buffer Refresh                      ║
    // ╚════════════════════════════════════════════════════════════════════════╝
    // Step 1: Shift existing 75 samples left
    for (uint8_t i = NEW_SAMPLE_COUNT; i < SPO2_BUFFER_LEN; i++) {
        redBuffer[i - NEW_SAMPLE_COUNT] = redBuffer[i];
        irBuffer[i  - NEW_SAMPLE_COUNT] = irBuffer[i];
    }

    // Step 2: Acquire 25 fresh samples from hardware FIFO
    for (uint8_t i = (SPO2_BUFFER_LEN - NEW_SAMPLE_COUNT); i < SPO2_BUFFER_LEN; i++) {
        while (particleSensor.available() == 0) {
            particleSensor.check();
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
        particleSensor.nextSample();
    }

    // ╔════════════════════════════════════════════════════════════════════════╗
    // ║  TASK B — Run the SpO2 & Heart Rate Algorithm                         ║
    // ╚════════════════════════════════════════════════════════════════════════╝
    maxim_heart_rate_and_oxygen_saturation(
        irBuffer,  SPO2_BUFFER_LEN,
        redBuffer,
        &spo2Raw,      &spo2ValidFlag,
        &heartRateRaw, &heartRateFlag
    );

    // Physiological filtering and caching
    if (heartRateFlag == 1 && heartRateRaw > 40 && heartRateRaw < 220) {
        displayHR   = heartRateRaw;
        hrGoodValue = true;
    }

    if (spo2ValidFlag == 1 && spo2Raw > 70 && spo2Raw <= 100) {
        displaySpO2   = spo2Raw;
        spo2GoodValue = true;
    }

    // ╔════════════════════════════════════════════════════════════════════════╗
    // ║  TASK C — MLX90614 Temperature Read (every TEMP_REFRESH_MS)           ║
    // ╚════════════════════════════════════════════════════════════════════════╝
    unsigned long now = millis();
    if (now - lastTempRead >= TEMP_REFRESH_MS) {
        lastTempRead = now;
        displayTempC = mlx.readObjectTempC();
    }

    // ╔════════════════════════════════════════════════════════════════════════╗
    // ║  TASK D — OLED Display Refresh (every DISPLAY_REFRESH_MS)             ║
    // ╚════════════════════════════════════════════════════════════════════════╝
    now = millis();
    if (now - lastDisplayRefresh >= DISPLAY_REFRESH_MS) {
        lastDisplayRefresh = now;
        updateDisplay();
    }

    // ╔════════════════════════════════════════════════════════════════════════╗
    // ║  TASK E — Blynk Cloud Telemetry & Alert Engine (every BLYNK_PUSH_MS)  ║
    // ╚════════════════════════════════════════════════════════════════════════╝
    now = millis();
    if (now - lastBlynkPush >= BLYNK_PUSH_MS) {
        lastBlynkPush = now;

        // Push data to Virtual Pins
        sendDataToBlynk();

        // Evaluate vitals against safety thresholds
        evaluateSafetyAlerts();
    }
}


// ============================================================================
//  SECTION 9 — BLYNK CLOUD TELEMETRY & ALERTS
// ============================================================================

/**
 * @brief  Pushes biometric values to Blynk Virtual Pins.
 *
 *  Transmits data only if connected to avoid unnecessary queue allocations.
 */
void sendDataToBlynk() {
    if (!Blynk.connected()) {
        return;
    }

    // Only send biometric numbers if valid data has been obtained
    if (hrGoodValue) {
        Blynk.virtualWrite(VPIN_HEART_RATE, displayHR);
    }
    if (spo2GoodValue) {
        Blynk.virtualWrite(VPIN_SPO2, displaySpO2);
    }

    // Body temperature from MLX90614 is continuously valid
    Blynk.virtualWrite(VPIN_BODY_TEMP, displayTempC);

    Serial.printf("[BLYNK] Telemetry pushed: HR=%d bpm | SpO2=%d %% | Temp=%.1f C\n",
                  hrGoodValue ? displayHR : 0,
                  spo2GoodValue ? displaySpO2 : 0,
                  displayTempC);
}

/**
 * @brief  Checks biometric values against hazardous environmental/health limits.
 *
 *  Triggers Blynk.logEvent("safety_alert", ...) with an enforcement cooldown
 *  so notifications are not repeatedly fired on every evaluation cycle.
 */
void evaluateSafetyAlerts() {
    bool isEmergency = false;
    String alertReason = "";

    // 1. High Heart Rate Check (> 110 BPM)
    if (hrGoodValue && displayHR > THRESHOLD_MAX_HR) {
        isEmergency = true;
        alertReason += "High HR (" + String(displayHR) + " bpm) ";
    }

    // 2. High Body Temperature / Heat Stress Check (> 38.0 °C)
    if (displayTempC > THRESHOLD_MAX_TEMP) {
        isEmergency = true;
        alertReason += "High Temp (" + String(displayTempC, 1) + "C) ";
    }

    // 3. Hypoxia / Low Oxygen Saturation Check (< 92%)
    if (spo2GoodValue && displaySpO2 < THRESHOLD_MIN_SPO2) {
        isEmergency = true;
        alertReason += "Low SpO2 (" + String(displaySpO2) + "%) ";
    }

    // Update Virtual Pin 4 Status Label on the Dashboard
    if (Blynk.connected()) {
        if (isEmergency) {
            Blynk.virtualWrite(VPIN_ALERT_STATUS, "ALERT: " + alertReason);
        } else {
            Blynk.virtualWrite(VPIN_ALERT_STATUS, "Vitals Normal");
        }
    }

    // ── COOLDOWN LOGIC EXPLANATION ──────────────────────────────────────────
    // If an emergency is detected, we check whether at least ALERT_COOLDOWN_MS
    // (60 seconds) has elapsed since the previous alert was logged.
    // Without this guard, evaluateSafetyAlerts() would execute Blynk.logEvent()
    // every 5 seconds while vitals stay critical, rapidly exhausting Blynk's
    // rate limits (100 events/device/day) and spamming the worker's supervisor.
    if (isEmergency) {
        unsigned long currentMillis = millis();

        if (currentMillis - lastAlertTrigger >= ALERT_COOLDOWN_MS) {
            lastAlertTrigger = currentMillis;  // Reset cooldown timer

            String alertMessage = "Worker Hazard Alert! " + alertReason;
            Serial.println(F("\n========================================"));
            Serial.println(F(" [CRITICAL ALERT] Dispatched to Blynk:"));
            Serial.println("  " + alertMessage);
            Serial.println(F("========================================\n"));

            if (Blynk.connected()) {
                // Event code must match the Event identifier configured in Blynk console
                Blynk.logEvent("safety_alert", alertMessage);
            }
        } else {
            unsigned long remainingSeconds = (ALERT_COOLDOWN_MS - (currentMillis - lastAlertTrigger)) / 1000;
            Serial.printf("[ALERT] Emergency condition active (%s), but cooldown in effect (%lu s remaining)\n",
                          alertReason.c_str(), remainingSeconds);
        }
    }
}


// ============================================================================
//  INITIALIZATION FUNCTIONS
// ============================================================================

void initOLED() {
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        Serial.println(F("[FATAL] SSD1306 OLED not found at 0x3C!"));
        while (true) { delay(500); }
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(8, 4);
    display.println(F("  Safety Wearable"));
    display.setCursor(22, 18);
    display.println(F("Phase 3 (IoT)"));
    display.drawFastHLine(0, 30, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(14, 38);
    display.println(F("Initializing..."));
    display.setCursor(4, 52);
    display.println(F("Checking sensors..."));
    display.display();
    Serial.println(F("[OK]   OLED (SSD1306) initialized at address 0x3C."));
    delay(1000);
}

void initMLX90614() {
    if (!mlx.begin()) {
        Serial.println(F("[FATAL] MLX90614 not found at 0x5A!"));
        displayError("MLX90614 FAIL", "I2C addr 0x5A absent");
        while (true) { delay(500); }
    }
    Serial.println(F("[OK]   MLX90614 initialized at address 0x5A."));
    displayTempC = mlx.readObjectTempC();
}

void initMAX30102() {
    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
        Serial.println(F("[FATAL] MAX30102 not found at 0x57!"));
        displayError("MAX30102 FAIL", "I2C addr 0x57 absent");
        while (true) { delay(500); }
    }

    const byte ledBrightness = 0x7F;  // ~25.4mA LED current
    const byte sampleAverage = 4;     // 4x hardware averaging -> 25 sps
    const byte ledMode       = 2;     // SpO2 mode: Red + IR
    const int  sampleRate    = 100;   // 100 raw sps
    const int  pulseWidth    = 411;   // 411µs pulse width -> 18-bit ADC
    const int  adcRange      = 4096;  // Full scale ADC range (nA)

    particleSensor.setup(
        ledBrightness,
        sampleAverage,
        ledMode,
        sampleRate,
        pulseWidth,
        adcRange
    );

    particleSensor.setPulseAmplitudeRed(0x7F);
    particleSensor.setPulseAmplitudeIR(0x7F);
    particleSensor.setPulseAmplitudeGreen(0x00);

    Serial.println(F("[OK]   MAX30102 initialized at address 0x57."));
}

void primeSensorBuffers() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(4, 2);
    display.println(F("Place finger firmly"));
    display.println(F("  on MAX30102 sensor."));
    display.drawFastHLine(0, 22, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(10, 28);
    display.println(F("Hold still while"));
    display.setCursor(10, 38);
    display.println(F("signal is acquired."));
    display.setCursor(10, 52);
    display.println(F("~4 seconds..."));
    display.display();

    Serial.println(F("\n[INFO] Priming MAX30102 buffer (100 samples)..."));

    for (uint8_t i = 0; i < SPO2_BUFFER_LEN; i++) {
        while (particleSensor.available() == 0) {
            particleSensor.check();
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
        particleSensor.nextSample();

        if ((i + 1) % 20 == 0) {
            Serial.printf("       [%3d/100] IR raw: %lu\n", i + 1, irBuffer[i]);
        }
    }
    Serial.println(F("[INFO] Buffer primed. Ready for live processing.\n"));
}


// ============================================================================
//  DISPLAY RENDERING FUNCTIONS
// ============================================================================

void updateDisplay() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // ── Heart Rate Block ──────────────────────────────────────────────────────
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("HR:"));

    display.setTextSize(2);
    display.setCursor(0, 10);
    if (hrGoodValue) {
        display.print(displayHR);
    } else {
        display.print(F("--"));
    }
    display.setTextSize(1);
    display.setCursor(display.getCursorX() + 3, 20);
    display.print(F("bpm"));

    // ── SpO2 Block ────────────────────────────────────────────────────────────
    display.setTextSize(1);
    display.setCursor(0, 30);
    display.print(F("SpO2:"));

    display.setTextSize(2);
    display.setCursor(0, 38);
    if (spo2GoodValue) {
        display.print(displaySpO2);
    } else {
        display.print(F("--"));
    }
    display.setTextSize(1);
    display.setCursor(display.getCursorX() + 3, 48);
    display.print(F("%"));

    // ── Wi-Fi / Cloud Indicator in Top Right ──────────────────────────────────
    display.setTextSize(1);
    display.setCursor(95, 0);
    if (Blynk.connected()) {
        display.print(F("[IoT]"));
    } else {
        display.print(F("[OFF]"));
    }

    // ── Temperature Footer ────────────────────────────────────────────────────
    display.drawFastHLine(0, 54, SCREEN_WIDTH, SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Temp: "));
    display.print(displayTempC, 1);
    display.print(F(" "));

    drawDegreeSymbol(display.getCursorX(), 56);

    display.setCursor(display.getCursorX() + 7, 56);
    display.print(F("C"));

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
    display.setCursor(4, 5);
    display.println(F(" !! FATAL ERROR !!"));
    display.drawFastHLine(0, 16, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(4, 22);
    display.println(title);
    display.setCursor(4, 36);
    display.println(msg);
    display.drawFastHLine(0, 49, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(4, 52);
    display.println(F("See Serial Monitor"));
    display.display();
}
