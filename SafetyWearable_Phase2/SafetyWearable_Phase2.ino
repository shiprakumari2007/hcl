/**
 * ============================================================================
 * Occupational Safety Wearable — Phase 2 Firmware
 * ============================================================================
 * Target MCU  : ESP32 38-pin (ESP32-WROOM-32 / DevKit V1)
 * Sensors     : MAX30102 (HR + SpO2) | MLX90614 (IR Temp)
 * Display     : 0.96" SSD1306 OLED (I2C)
 * I2C Pins    : SDA = GPIO 21 | SCL = GPIO 22
 * I2C Speed   : 100 kHz (SMBus compatible — limited by MLX90614)
 *
 * ============================================================================
 * REQUIRED LIBRARIES — Install via: Arduino IDE ▶ Tools ▶ Manage Libraries
 * ============================================================================
 *
 *  1. "SparkFun MAX3010x Pulse and Proximity Sensor Library"
 *     Author  : SparkFun Electronics
 *     Search  : MAX3010x
 *     Version : 1.1.2 or newer
 *     Provides: MAX30105.h   ← The sensor driver (also covers MAX30102)
 *               heartRate.h  ← Simple beat-detection utility (unused here)
 *               spo2_algorithm.h ← Maxim reference SpO2 algorithm
 *
 *  2. "Adafruit MLX90614 Library"
 *     Author  : Adafruit
 *     Search  : MLX90614
 *     Version : 2.1.5 or newer
 *     Provides: Adafruit_MLX90614.h
 *
 *  3. "Adafruit SSD1306"
 *     Author  : Adafruit
 *     Search  : Adafruit SSD1306
 *     Version : 2.5.9 or newer
 *     Provides: Adafruit_SSD1306.h
 *
 *  4. "Adafruit GFX Library"
 *     Author  : Adafruit
 *     Search  : Adafruit GFX
 *     Version : 1.11.9 or newer
 *     Provides: Adafruit_GFX.h
 *     NOTE    : This is a mandatory dependency of library #3.
 *               The Arduino IDE may offer to install it automatically.
 *               If not prompted, install it manually.
 *
 * ============================================================================
 * ARDUINO IDE BOARD SETTINGS
 * ============================================================================
 *  Board           : "ESP32 Dev Module"
 *  Upload Speed    : 921600
 *  CPU Frequency   : 240MHz (or 160MHz to save power)
 *  Flash Frequency : 80MHz
 *  Partition Scheme: Default 4MB with spiffs
 * ============================================================================
 */

// ─── Standard & Platform Libraries ───────────────────────────────────────────
#include <Wire.h>               // Arduino I2C master library

// ─── Sensor Libraries ────────────────────────────────────────────────────────
#include <MAX30105.h>           // SparkFun MAX3010x driver (covers MAX30102)
#include <spo2_algorithm.h>     // Maxim reference SpO2 + HR algorithm
#include <Adafruit_MLX90614.h>  // Adafruit MLX90614 IR thermometer driver

// ─── Display Libraries ───────────────────────────────────────────────────────
#include <Adafruit_GFX.h>       // Adafruit graphics primitives base class
#include <Adafruit_SSD1306.h>   // Adafruit SSD1306 OLED driver


// ============================================================================
//  SECTION 1 — HARDWARE CONSTANTS
// ============================================================================

// I2C Pin Assignment (ESP32 38-pin hardware I2C peripheral)
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22

// OLED Display Geometry & Address
#define SCREEN_WIDTH        128   // SSD1306 horizontal resolution (pixels)
#define SCREEN_HEIGHT        64   // SSD1306 vertical resolution (pixels)
#define OLED_RESET_PIN       -1   // -1 = share the ESP32 EN/RESET line
#define OLED_I2C_ADDR      0x3C  // Most common: 0x3C. If not found, try 0x3D.

// I2C Bus Speed: 100 kHz
// CRITICAL: The MLX90614 uses the SMBus protocol, which has a MAXIMUM clock
// speed of 100kHz. The MAX30102 supports 400kHz, but since we share the bus,
// the slowest device governs. Do NOT set this above 100000.
#define I2C_BUS_SPEED      100000UL


// ============================================================================
//  SECTION 2 — TIMING CONSTANTS
// ============================================================================

// How often the OLED refreshes (ms). 1000ms = 1 Hz.
// This intentionally decoupled from sensor sampling — the MAX30102
// runs continuously at its own hardware rate regardless of this timer.
#define DISPLAY_REFRESH_MS  1000UL

// How often MLX90614 is polled (ms). The MLX90614 returns a new reading
// every ~38ms internally, but for thermal monitoring 2 seconds is plenty
// and reduces I2C bus traffic during the critical MAX30102 sampling window.
#define TEMP_REFRESH_MS     2000UL


// ============================================================================
//  SECTION 3 — MAX30102 SpO2 ALGORITHM CONSTANTS
// ============================================================================

// The Maxim SpO2 algorithm requires a fixed window of exactly 100 samples.
// This is a hard requirement of the spo2_algorithm implementation.
#define SPO2_BUFFER_LEN     100

// On each main loop iteration we "slide" the window:
//   - Discard the oldest NEW_SAMPLE_COUNT entries from the front.
//   - Acquire NEW_SAMPLE_COUNT new samples from the sensor FIFO.
//   - Re-run the algorithm on the updated 100-sample window.
// At 100sps / 4x hardware average = 25 effective samples/sec.
// 25 new samples = ~1 second of fresh data per algorithm call.
// Using 25 (not 100) means the HR/SpO2 result updates ~every 1 second,
// which matches our display refresh — a good trade-off for wearables.
#define NEW_SAMPLE_COUNT     25


// ============================================================================
//  SECTION 4 — SENSOR OBJECTS
// ============================================================================

// MAX30102: SparkFun library uses "MAX30105" as the class name for both
// the MAX30105 and MAX30102. They are register-compatible. The begin()
// call confirms the specific device ID via I2C register 0xFF.
MAX30105          particleSensor;

// MLX90614: Adafruit driver. begin() returns false if device not found.
Adafruit_MLX90614 mlx;

// SSD1306 OLED: Passed a reference to the Wire object so it shares
// the same I2C bus instance we configure in setup().
Adafruit_SSD1306  display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);


// ============================================================================
//  SECTION 5 — MAX30102 RAW DATA BUFFERS
// ============================================================================

// 32-bit unsigned buffers to hold raw photodiode counts from the sensor FIFO.
// IR channel   → used for heart rate detection (primary signal).
// Red channel  → used together with IR to compute the R ratio for SpO2.
uint32_t irBuffer[SPO2_BUFFER_LEN];
uint32_t redBuffer[SPO2_BUFFER_LEN];


// ============================================================================
//  SECTION 6 — ALGORITHM RESULTS (raw, from spo2_algorithm)
// ============================================================================

int32_t spo2Raw         = 0;   // SpO2 value returned by the algorithm (%)
int8_t  spo2ValidFlag   = 0;   // 1 = algorithm reports HIGH confidence result
int32_t heartRateRaw    = 0;   // Heart rate returned by the algorithm (BPM)
int8_t  heartRateFlag   = 0;   // 1 = algorithm reports HIGH confidence result


// ============================================================================
//  SECTION 7 — DISPLAY CACHE
// ============================================================================
// The display only updates at 1Hz. We cache the LAST KNOWN GOOD reading
// here so the screen never regresses to "--" after a momentary invalid
// algorithm result (e.g., user shifts their finger slightly).
//
// hrGoodValue / spo2GoodValue toggle to false only on startup (before
// the first valid reading is received).

int32_t displayHR       = 0;
int32_t displaySpO2     = 0;
float   displayTempC    = 0.0f;
bool    hrGoodValue     = false;
bool    spo2GoodValue   = false;


// ============================================================================
//  SECTION 8 — NON-BLOCKING TIMER STATE
// ============================================================================

unsigned long lastDisplayRefresh  = 0;  // Timestamp of last OLED draw
unsigned long lastTempRead        = 0;  // Timestamp of last MLX90614 read


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


// ============================================================================
//  SETUP — Runs once on power-on or reset
// ============================================================================

void setup() {
    // Initialize Serial Monitor for debug output
    Serial.begin(115200);
    delay(150);  // Allow USB-Serial bridge to enumerate

    Serial.println(F("\n============================================"));
    Serial.println(F("  Occupational Safety Wearable — Phase 2"));
    Serial.println(F("============================================"));
    Serial.printf ("  Build date: %s %s\n", __DATE__, __TIME__);
    Serial.println();

    // ── Initialize I2C Bus ───────────────────────────────────────────────────
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(I2C_BUS_SPEED);
    Serial.printf("[BUS] I2C initialized: SDA=GPIO%d, SCL=GPIO%d, Speed=%lu Hz\n",
                  I2C_SDA_PIN, I2C_SCL_PIN, I2C_BUS_SPEED);

    // ── Initialize Peripherals (order matters: OLED first for error display) ─
    initOLED();
    initMLX90614();
    initMAX30102();

    // ── Prime the SpO2 buffer ────────────────────────────────────────────────
    // This is the ONLY intentionally blocking call in the entire firmware.
    // The spo2_algorithm cannot produce any output without a full 100-sample
    // history. We fill it here, once, with user guidance shown on the OLED.
    primeSensorBuffers();

    Serial.println(F("\n[READY] System online. Non-blocking loop starting.\n"));
}


// ============================================================================
//  MAIN LOOP — Fully Non-Blocking via millis() scheduling
// ============================================================================
//
//  Four concurrent "tasks" run here without any delay():
//
//   Task A: Sliding-Window Buffer Refresh  ─┐
//   Task B: SpO2 + HR Algorithm             ├─ Run every iteration (fast)
//   Task C: MLX90614 Temperature Read      ─── Every TEMP_REFRESH_MS
//   Task D: OLED Display Refresh           ─── Every DISPLAY_REFRESH_MS
//
// ============================================================================

void loop() {

    // ╔════════════════════════════════════════════════════════════════════════╗
    // ║  TASK A — MAX30102 Sliding Window Buffer Refresh                      ║
    // ╚════════════════════════════════════════════════════════════════════════╝
    //
    // The algorithm needs 100 samples. Instead of re-collecting all 100 each
    // cycle (which would block for ~4 seconds), we:
    //
    //  Step 1. Shift: Move samples [25..99] → [0..74] (discard oldest 25)
    //  Step 2. Fill:  Acquire 25 fresh samples into positions [75..99]
    //  Step 3. Run algorithm on the updated 100-sample window.
    //
    // This keeps the algorithm running with a continuously rolling dataset.
    // The inner while() briefly yields until a new sample appears in the FIFO
    // — it does NOT spin forever, as the sensor produces data at 100sps.

    // Step 1: Shift the existing buffer left by NEW_SAMPLE_COUNT positions
    for (uint8_t i = NEW_SAMPLE_COUNT; i < SPO2_BUFFER_LEN; i++) {
        redBuffer[i - NEW_SAMPLE_COUNT] = redBuffer[i];
        irBuffer[i  - NEW_SAMPLE_COUNT] = irBuffer[i];
    }

    // Step 2: Collect NEW_SAMPLE_COUNT (25) fresh samples from the sensor FIFO
    for (uint8_t i = (SPO2_BUFFER_LEN - NEW_SAMPLE_COUNT); i < SPO2_BUFFER_LEN; i++) {

        // particleSensor.available() returns the number of unread samples in
        // the library's internal FIFO mirror. If it is 0, we call check()
        // to trigger an I2C read of the sensor hardware FIFO and repopulate it.
        while (particleSensor.available() == 0) {
            particleSensor.check();  // Non-blocking I2C burst read from MAX30102
        }

        redBuffer[i] = particleSensor.getRed();   // Read Red channel ADC count
        irBuffer[i]  = particleSensor.getIR();    // Read IR  channel ADC count
        particleSensor.nextSample();              // Advance library FIFO read pointer
    }


    // ╔════════════════════════════════════════════════════════════════════════╗
    // ║  TASK B — Run the SpO2 & Heart Rate Algorithm                         ║
    // ╚════════════════════════════════════════════════════════════════════════╝
    //
    // maxim_heart_rate_and_oxygen_saturation() is Maxim's reference C
    // implementation, ported by SparkFun. It analyzes the
    // photoplethysmography (PPG) waveform in both IR and Red channels:
    //
    //   • Heart Rate  → Detected by finding peaks in the IR AC waveform.
    //   • SpO2        → Computed from the ratio-of-ratios (R):
    //                   R = (AC_red/DC_red) / (AC_ir/DC_ir)
    //                   SpO2 ≈ -45.060*R² + 30.354*R + 94.845  (calibration curve)
    //
    // Validity flags: 1 = high confidence. 0 = signal too weak/noisy.
    //                 Possible causes: no finger, motion artifact, weak pressure.

    maxim_heart_rate_and_oxygen_saturation(
        irBuffer,  SPO2_BUFFER_LEN,   // IR channel buffer + length
        redBuffer,                    // Red channel buffer
        &spo2Raw,      &spo2ValidFlag,   // OUTPUT: SpO2 value + confidence flag
        &heartRateRaw, &heartRateFlag    // OUTPUT: HR value  + confidence flag
    );

    // ── Cache results: only update display values on valid + physiological reads
    // Sanity bounds for heart rate: 40–220 BPM (valid for adults under exertion)
    if (heartRateFlag == 1 && heartRateRaw > 40 && heartRateRaw < 220) {
        displayHR   = heartRateRaw;
        hrGoodValue = true;
    }

    // Sanity bounds for SpO2: 70–100% (below 70 is medically severe hypoxia)
    if (spo2ValidFlag == 1 && spo2Raw > 70 && spo2Raw <= 100) {
        displaySpO2   = spo2Raw;
        spo2GoodValue = true;
    }


    // ╔════════════════════════════════════════════════════════════════════════╗
    // ║  TASK C — MLX90614 Temperature Read (every TEMP_REFRESH_MS)           ║
    // ╚════════════════════════════════════════════════════════════════════════╝
    //
    // We read temperature on a slower 2-second schedule. This is deliberate:
    //   1. Temperature changes slowly — sub-second resolution is wasteful.
    //   2. Every I2C transaction on the shared bus briefly pauses the CPU.
    //      Keeping MLX reads infrequent maximizes time for MAX30102 FIFO reads.
    //
    // readObjectTempC()   → Returns the IR-measured object (skin) temperature.
    // readAmbientTempC()  → Returns the sensor's own housing temperature (NOT skin).

    unsigned long now = millis();
    if (now - lastTempRead >= TEMP_REFRESH_MS) {
        lastTempRead  = now;
        displayTempC  = mlx.readObjectTempC();
    }


    // ╔════════════════════════════════════════════════════════════════════════╗
    // ║  TASK D — OLED Display Refresh (every DISPLAY_REFRESH_MS)             ║
    // ╚════════════════════════════════════════════════════════════════════════╝

    now = millis();  // Re-read for accuracy after tasks A–C execution time
    if (now - lastDisplayRefresh >= DISPLAY_REFRESH_MS) {
        lastDisplayRefresh = now;

        // ── Serial Monitor Debug Output ───────────────────────────────────
        Serial.printf("[DATA] HR: %-5s bpm | SpO2: %-4s %% | Temp: %.1f C | IR_raw: %lu\n",
            hrGoodValue   ? String(displayHR).c_str()   : "--",
            spo2GoodValue ? String(displaySpO2).c_str() : "--",
            displayTempC,
            irBuffer[SPO2_BUFFER_LEN - 1]   // Last raw IR value — useful for
                                             // diagnosing sensor contact quality:
                                             // < 50,000 = weak/no contact
                                             // 50,000–200,000 = good signal
                                             // > 200,000 = may be clipping (reduce LED)
        );

        updateDisplay();  // Render the frame
    }
}


// ============================================================================
//  INITIALIZATION FUNCTIONS
// ============================================================================

/**
 * @brief  Initialize the SSD1306 OLED display and show a boot splash screen.
 *
 * @note   This function MUST be called first in setup() before any other sensor
 *         init. This ensures that if a subsequent sensor fails, the error can
 *         be shown both on Serial AND on the OLED display.
 *
 * @halt   If the OLED is not found, the program halts permanently (infinite
 *         loop). Without a display, the device cannot show readings — there
 *         is no graceful fallback in this wearable context.
 */
void initOLED() {
    // SSD1306_SWITCHCAPVCC: Use the module's internal charge pump to generate
    // the 7.5V OLED panel voltage from the 3.3V supply. This is the standard
    // mode for standalone OLED modules with no external VPP supply.
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
        Serial.println(F("[FATAL] SSD1306 OLED not found!"));
        Serial.printf ("         Expected I2C address: 0x%02X\n", OLED_I2C_ADDR);
        Serial.println(F("         ▶ Check: VCC=3.3V, GND, SDA=GPIO21, SCL=GPIO22"));
        Serial.println(F("         ▶ If your module has an address jumper, try 0x3D"));
        Serial.println(F("[FATAL] System HALTED."));
        while (true) { delay(500); }  // Halt permanently
    }

    // Show boot splash
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(8, 4);
    display.println(F("  Safety Wearable"));

    display.setTextSize(1);
    display.setCursor(22, 18);
    display.println(F("Phase 2  v1.0"));

    // Draw a decorative separator
    display.drawFastHLine(0, 30, SCREEN_WIDTH, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(14, 38);
    display.println(F("Initializing..."));
    display.setCursor(4, 52);
    display.println(F("Checking sensors..."));

    display.display();
    Serial.println(F("[OK]   OLED (SSD1306) initialized at address 0x3C."));

    delay(1200);  // Display splash for 1.2 seconds — only delay() in this file
                  // that is architecturally justified (startup UX, not sensor timing).
}

/**
 * @brief  Initialize the MLX90614 IR body temperature sensor.
 *
 * @note   The MLX90614 is polled at 0x5A. The Adafruit driver verifies
 *         communication by reading an EEPROM register on begin().
 *
 * @halt   Displays an error on OLED + Serial and halts on failure.
 */
void initMLX90614() {
    if (!mlx.begin()) {
        Serial.println(F("[FATAL] MLX90614 not found at 0x5A!"));
        Serial.println(F("         ▶ Check: VDD=3.3V (NOT 5V!), GND, SDA, SCL"));
        Serial.println(F("         ▶ Verify pull-up resistors present on SDA/SCL"));
        displayError("MLX90614 FAIL", "I2C addr 0x5A absent");
        while (true) { delay(500); }
    }

    Serial.println(F("[OK]   MLX90614 initialized at address 0x5A."));
    Serial.printf( "        Emissivity (should be ~1.0): %.3f\n", mlx.readEmissivity());

    // Pre-read the first temperature so displayTempC is valid before loop() starts
    displayTempC = mlx.readObjectTempC();
}

/**
 * @brief  Initialize the MAX30102 with wearable-optimized configuration.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 *  Configuration Reference Table:
 * ─────────────────────────────────────────────────────────────────────────────
 *
 *  ledBrightness = 0x7F (127 decimal → ~25.4 mA)
 *  ───────────────────────────────────────────────
 *  The MAX30102 LED current register maps 0x00–0xFF to 0–51mA (0.2mA/LSB).
 *  0x7F = 127 × 0.2mA = 25.4mA per LED channel.
 *
 *  WHY 25.4mA:
 *   • Construction workers often have thick, callused, or darkly pigmented skin
 *     on their fingertips. Both conditions absorb more light, requiring higher
 *     LED power to achieve a measurable signal at the photodiode.
 *   • If your raw IR reading (printed to Serial) is consistently > 200,000
 *     counts, the signal is saturating — reduce this value to 0x4F (~15.8mA).
 *   • If your IR reading is consistently < 50,000 counts with finger applied,
 *     increase this value toward 0xBF (~38.2mA).
 *
 *  sampleAverage = 4
 *  ───────────────────
 *  The MAX30102's onboard DSP averages N consecutive raw ADC samples before
 *  placing one result into the FIFO. This is hardware-side averaging.
 *  Effective output rate = sampleRate / sampleAverage = 100 / 4 = 25 sps.
 *  25 Hz is more than adequate to capture PPG peaks (heart beats at 0.6–3 Hz).
 *  This reduces high-frequency noise without consuming CPU cycles for averaging.
 *
 *  ledMode = 2 (SpO2 Mode)
 *  ───────────────────────
 *  Mode 1 = IR only          → Heart rate only (1 channel)
 *  Mode 2 = Red + IR         → SpO2 + Heart rate (2 channels) ← We use this
 *  Mode 3 = Red + IR + Green → Proximity mode (MAX30105 only; no Green on MAX30102)
 *
 *  sampleRate = 100 sps
 *  ─────────────────────
 *  100 raw ADC conversions per second. Combined with 4x averaging,
 *  the algorithm sees 25 unique data points per second.
 *  Valid options: 50, 100, 200, 400, 800, 1000, 1600, 3200 sps.
 *
 *  pulseWidth = 411 µs → 18-bit ADC resolution
 *  ─────────────────────────────────────────────
 *  The pulse width determines how long the LED illuminates per sample.
 *  Longer pulse → more photons collected → better Signal-to-Noise Ratio.
 *  411µs is the maximum available and gives the highest 18-bit ADC resolution.
 *  Valid options: 69µs (15-bit), 118µs (16-bit), 215µs (17-bit), 411µs (18-bit).
 *
 *  adcRange = 4096 nA
 *  ───────────────────
 *  Full-scale range of the photodiode transimpedance amplifier.
 *  4096 nA is the maximum, preventing ADC clipping under strong signals.
 *  Valid options: 2048, 4096, 8192, 16384 nA.
 *
 * ─────────────────────────────────────────────────────────────────────────────
 *
 * @halt   Displays an error on OLED + Serial and halts on failure.
 */
void initMAX30102() {
    // Pass I2C_SPEED_STANDARD to ensure MAX30102 does NOT override our
    // bus speed back to 400kHz (which would violate the MLX90614 limit).
    if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
        Serial.println(F("[FATAL] MAX30102 not found at 0x57!"));
        Serial.println(F("         ▶ Check: VCC=3.3V (NOT 5V!), GND, SDA, SCL"));
        Serial.println(F("         ▶ MAX30102 VCC MUST be 3.3V — 5V will destroy it!"));
        displayError("MAX30102 FAIL", "I2C addr 0x57 absent");
        while (true) { delay(500); }
    }

    // ── Sensor Configuration ─────────────────────────────────────────────────
    const byte ledBrightness = 0x7F;  // ~25.4mA LED current (see above for tuning guide)
    const byte sampleAverage = 4;     // 4x hardware averaging → effective 25 sps output
    const byte ledMode       = 2;     // SpO2 mode: Red + IR (both channels active)
    const int  sampleRate    = 100;   // 100 raw ADC samples/second
    const int  pulseWidth    = 411;   // 411µs pulse width → 18-bit ADC resolution
    const int  adcRange      = 4096;  // Full-scale ADC range (nA); prevents saturation

    particleSensor.setup(
        ledBrightness,
        sampleAverage,
        ledMode,
        sampleRate,
        pulseWidth,
        adcRange
    );

    // ── Fine-Tune LED Amplitude ───────────────────────────────────────────────
    // The setup() call above sets the primary LED amplitude. We also explicitly
    // set Red and IR to the same value to ensure both channels are balanced.
    // An unbalanced Red/IR ratio biases the SpO2 ratio-of-ratios calculation.
    particleSensor.setPulseAmplitudeRed(0x7F);   // Red LED: ~25.4mA
    particleSensor.setPulseAmplitudeIR(0x7F);    // IR  LED: ~25.4mA
    particleSensor.setPulseAmplitudeGreen(0x00); // Green LED: OFF (Mode 2 unused)

    Serial.println(F("[OK]   MAX30102 initialized at address 0x57."));
    Serial.println(F("        Config: LED=25.4mA | Avg=4x | Mode=SpO2 | 100sps | PW=411µs | ADC=4096nA"));
}


// ============================================================================
//  SENSOR BUFFER PRIMING
// ============================================================================

/**
 * @brief  Block until the 100-sample MAX30102 buffer is fully populated.
 *
 * This function is called once in setup(). It is the ONLY intentional blocking
 * operation in the firmware. The Maxim spo2_algorithm requires a complete
 * 100-sample history to compute its first result.
 *
 * At 25 effective samples/second (100sps / 4x avg), acquiring 100 samples
 * takes approximately 4 seconds. The user is prompted to place their finger.
 *
 * A Serial progress counter is printed every 10 samples to aid debugging
 * in cases where the sensor is not making proper skin contact.
 */
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

    Serial.println(F("\n[INFO] Priming MAX30102 buffer (100 samples required)..."));
    Serial.println(F("       ▶ Place your finger firmly on the MAX30102 now."));
    Serial.println(F("       ▶ IR raw count should be > 50,000 for good contact.\n"));

    for (uint8_t i = 0; i < SPO2_BUFFER_LEN; i++) {
        // Wait for the next available sample in the library's FIFO mirror
        while (particleSensor.available() == 0) {
            particleSensor.check();
        }
        redBuffer[i] = particleSensor.getRed();
        irBuffer[i]  = particleSensor.getIR();
        particleSensor.nextSample();

        // Progress output to Serial every 10 samples
        if ((i + 1) % 10 == 0) {
            Serial.printf("       [%3d/100] IR raw: %lu %s\n",
                i + 1,
                irBuffer[i],
                (irBuffer[i] < 50000) ? "← WEAK SIGNAL — check finger contact!" : ""
            );
        }
    }
    Serial.println(F("\n[INFO] Buffer primed successfully. Live readings starting.\n"));
}


// ============================================================================
//  DISPLAY FUNCTIONS
// ============================================================================

/**
 * @brief  Renders the main biometric dashboard on the SSD1306 OLED.
 *
 *  Screen layout (128 × 64 pixels):
 *  ┌──────────────────────────────┐
 *  │ HR:                          │  y=0   (TextSize 1 — label)
 *  │ 72  bpm                      │  y=10  (TextSize 2 — large value)
 *  │ SpO2:                        │  y=30  (TextSize 1 — label)
 *  │ 98  %                        │  y=38  (TextSize 2 — large value)
 *  │──────────────────────────────│  y=54  (Horizontal rule divider)
 *  │ Temp: 36.7 °C                │  y=56  (TextSize 1 — compact footer)
 *  └──────────────────────────────┘
 *
 *  TextSize 2 characters are 12px wide × 16px tall.
 *  A 3-digit number (e.g., "120") = 36px wide — fits comfortably with label.
 *  "--" is shown for each metric until the algorithm produces a valid result.
 */
void updateDisplay() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // ── HEART RATE BLOCK ──────────────────────────────────────────────────────
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
    // Print "bpm" unit in small text at baseline of the large number
    display.setTextSize(1);
    display.setCursor(display.getCursorX() + 3, 20);
    display.print(F("bpm"));

    // ── SpO2 BLOCK ────────────────────────────────────────────────────────────
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
    // Print "%" unit in small text at baseline
    display.setTextSize(1);
    display.setCursor(display.getCursorX() + 3, 48);
    display.print(F("%"));

    // ── TEMPERATURE FOOTER ────────────────────────────────────────────────────
    // A horizontal rule visually separates temp from the HR/SpO2 blocks above
    display.drawFastHLine(0, 54, SCREEN_WIDTH, SSD1306_WHITE);

    display.setTextSize(1);
    display.setCursor(0, 56);
    display.print(F("Temp: "));
    display.print(displayTempC, 1);  // 1 decimal place (e.g., "36.7")
    display.print(F(" "));           // Space before degree symbol

    // Draw a custom degree symbol (°). The SSD1306 default font only covers
    // printable ASCII (0x20–0x7E). Characters beyond 0x7F are not defined
    // in the Adafruit GFX 5×7 font, so we draw the ° as a 4px hollow circle.
    drawDegreeSymbol(display.getCursorX(), 56);

    // Advance cursor past the manually drawn circle (diameter ~5px + 1px gap)
    display.setCursor(display.getCursorX() + 7, 56);
    display.print(F("C"));

    // Push the completed framebuffer to the physical display via I2C.
    // This single call sends 128*64/8 = 1024 bytes over I2C — takes ~1ms at 100kHz.
    display.display();
}

/**
 * @brief  Draw a small degree symbol (°) as a hollow circle.
 *
 *  The Adafruit GFX 5×7 pixel font does not include a degree character.
 *  We render one manually as a 4-pixel-diameter circle using drawCircle(),
 *  which draws a 1-pixel-wide circle outline.
 *
 * @param x  X pixel coordinate of the symbol's left edge
 * @param y  Y pixel coordinate of the symbol's top edge
 */
void drawDegreeSymbol(int16_t x, int16_t y) {
    // drawCircle(center_x, center_y, radius, color)
    // Center at (x+2, y+2), radius 2 → 5×5 pixel bounding box
    display.drawCircle(x + 2, y + 2, 2, SSD1306_WHITE);
}

/**
 * @brief  Render a prominent FATAL ERROR screen on the OLED.
 *
 *  Called when a sensor fails to initialize. The error is shown both here
 *  and on the Serial Monitor. The device is halted after this is displayed.
 *
 * @param title  Short error identifier (max ~20 chars at TextSize 1)
 * @param msg    Descriptive message (max ~20 chars at TextSize 1)
 */
void displayError(const char* title, const char* msg) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    // Outer border box — visually indicates a fatal condition
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

    // Error header
    display.setTextSize(1);
    display.setCursor(4, 5);
    display.println(F(" !! FATAL ERROR !!"));

    display.drawFastHLine(0, 16, SCREEN_WIDTH, SSD1306_WHITE);

    // Error title (e.g., "MAX30102 FAIL")
    display.setCursor(4, 22);
    display.setTextSize(1);
    display.println(title);

    // Descriptive message (e.g., "I2C addr 0x57 absent")
    display.setCursor(4, 36);
    display.println(msg);

    // Instruction footer
    display.drawFastHLine(0, 49, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(4, 52);
    display.println(F("See Serial Monitor"));

    display.display();
}
