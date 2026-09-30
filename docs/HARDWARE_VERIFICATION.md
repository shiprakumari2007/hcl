# Hardware Verification & Power Architecture Assessment

**Project:** P_174 — Occupational Safety & Health Monitoring System  
**Document Ref:** `docs/HARDWARE_VERIFICATION.md`  
**Target Hardware:** ESP32 38-Pin Development Board, MAX30102, MLX90614, SSD1306 0.96" OLED

---

## 1. Critical Warning on LiPo Powering via ESP32 VIN

> [!CAUTION]
> **Do NOT connect a single-cell 3.7V LiPo battery directly to the VIN / 5V pin of an unverified ESP32 development board.**

### The Technical Hazard (LDO Voltage Dropout Collapse)
A single-cell Lithium Polymer (LiPo) battery discharges from **4.20V (100% SOC)** down to **3.00V (0% SOC)**, spending the majority of its operational life between **3.60V and 3.80V**.

Most low-cost ESP32 DevKit boards (30-pin and 38-pin) employ an **AMS1117-3.3** Low Dropout (LDO) linear regulator:
* **AMS1117 Dropout Voltage ($V_{dropout}$):** Typically **1.1V to 1.3V** under load.
* **Required Input Voltage:** $V_{IN} \ge 3.3\text{V} + 1.1\text{V} = \mathbf{4.40\text{V}}$.

When a LiPo battery is at 3.7V:
$$V_{OUT} \approx 3.7\text{V} - 1.1\text{V} = \mathbf{2.60\text{V}}$$

The ESP32's internal Brownout Detector (BOD) trips when $V_{DD}$ drops below **2.43V–2.80V** (configurable trigger level 0 to 7). During Wi-Fi calibration bursts and MQTT transmissions, the ESP32 consumes peak currents exceeding **240mA**, triggering instantaneous brownout crashes and endless reboot loops (`rst:0x10 (RTCWDT_RTC_RESET)` or `Brownout detector was triggered`).

---

## 2. Hardware Identification & Board-Specific Matrix

| Sub-System | Hardware Variant | Verification Required | Risk Level |
| :--- | :--- | :--- | :--- |
| **ESP32 Board** | DevKit V1 38-Pin vs 30-Pin vs FireBeetle | Inspect IC marking on the 3-pin LDO near the USB connector. If marked `1117`, direct LiPo to VIN will fail. If marked `ME6211` or `RT9013` ($V_{drop} \approx 250\text{mV}$), LiPo to VIN is viable down to ~3.55V. | **HIGH** |
| **MLX90614** | `MLX90614ESF-BAA` (3.0V/3.3V) vs `MLX90614ESF-AAA` (5.0V) | Breakout boards purchased on hobbyist markets frequently ship with the 5V `AAA` suffix sensor. If fed with 3.3V, `AAA` variants report erroneous temperatures ($+10^\circ\text{C}$ to $+30^\circ\text{C}$) or fail I2C ACK. | **HIGH** |
| **MAX30102** | SparkFun breakout vs Generic RCWL-0515 clone | Clones often route VCC to an onboard 1.8V regulator for logic and LEDs. Confirm whether the breakout board expects 3.3V or 5V on its VCC pin. | **MEDIUM** |
| **OLED 0.96"** | SSD1306 4-pin I2C module | Confirm onboard pull-up resistors (typically 10kΩ or 4.7kΩ) and ensure jumper resistor for address selects `0x3C` (default) vs `0x3D`. | **LOW** |

---

## 3. Verified Power Options

```
                   SAFE PROTOTYPE POWER ARCHITECTURE
                   
  [3.7V LiPo Battery]
          │
          ▼
  [TP4056 with DW01A + FS8205A Protection IC]
     B+ / B- : Battery connection
     OUT+ / OUT- : Load output
          │ (3.0V - 4.2V Protected)
          ├─────────────────────────────────────────────────┐
          │                                                 │
   [OPTION 1: BOOST STAGE]                           [OPTION 2: ULTRA-LOW DROPOUT]
          │                                                 │
    MT3608 Boost Module                               Microchip MCP1700-3302E
    (Stepped up to 5.0V stable)                      (Ultra-low dropout: 178mV @ 250mA)
          │                                                 │
          ▼                                                 ▼
   ESP32 VIN Pin (5V)                               ESP32 3V3 Pin (Regulated Rail)
          │                                                 │
    Onboard LDO converts to 3.3V                            │
          │                                                 │
          └────────────────────────┬────────────────────────┘
                                   │
                                   ▼
                      [3.3V SENSOR POWER RAIL]
                     ┌─────────────┼─────────────┐
                     ▼             ▼             ▼
                 MAX30102       MLX90614       SSD1306
                 (Pin VCC)     (Pin VDD)      (Pin VCC)
```

### Option 1 (Recommended for Prototyping without Board Modification)
* Place an **MT3608 mini step-up boost converter** between TP4056 `OUT+` and ESP32 `VIN`.
* Adjust the trimpot so the output is exactly **5.00V DC**.
* The onboard AMS1117 receives 5.0V (headroom = $5.0 - 3.3 = 1.7\text{V} > 1.1\text{V}$), providing clean 3.3V to the core and sensors under all Wi-Fi load spikes.

### Option 2 (Recommended for Final Wearable Miniaturization)
* Connect TP4056 `OUT+` to an external **MCP1700-3302E** or **XC6206P332MR** ultra-low-dropout regulator ($V_{drop} \approx 170\text{mV}$).
* Route the regulated 3.3V directly to the ESP32 `3V3` pin and sensor VCC rail.
* Battery is usable down to $3.3\text{V} + 0.17\text{V} = \mathbf{3.47\text{V}}$ before dropout occurs.

---

## 4. I2C Bus & Electrical Logic Level Rules

1. **Common Rail Voltage:** All three I2C slaves (MAX30102, MLX90614, SSD1306) and the ESP32 GPIOs operate at **3.3V CMOS logic**. Never connect 5V to any sensor breakout board.
2. **I2C Clock Ceiling (SMBus Limitation):**
   * MAX30102 supports up to 400 kHz (Fast Mode).
   * SSD1306 supports up to 400 kHz.
   * **MLX90614 SMBus hardware interface has a hard upper clock limit of 100 kHz.**
   * **Rule:** The I2C master bus frequency MUST be configured to `Wire.setClock(100000);`.
3. **Pull-Up Resistor Calculation:**
   * Each breakout board features onboard pull-up resistors (typically 4.7kΩ to 10kΩ).
   * In parallel, 3 modules yield an effective resistance:
     $$\frac{1}{R_{eff}} = \frac{1}{4.7\text{k}\Omega} + \frac{1}{4.7\text{k}\Omega} + \frac{1}{10\text{k}\Omega} \implies R_{eff} \approx 1.9\text{k}\Omega$$
   * For short wires ($< 10\text{ cm}$), 1.9kΩ is acceptable. If bus errors occur, desolder the SMD pull-up resistors from two of the three boards.

---

## 5. Physical Verification Checklist for Engineers

- [ ] **LDO Verification:** Inspect ESP32 board regulator IC under a magnifying glass; confirm marking.
- [ ] **Sensor Suffix Check:** Check laser engraving on the MLX90614 TO-39 metal can. Confirm it reads `BAA` (3V), not `AAA` (5V).
- [ ] **TP4056 Ground Bonding:** Verify that TP4056 `OUT-` connects to the system common ground plane. Do not ground to `B-`.
- [ ] **Decoupling Capacitors:** Solder a 100nF ceramic capacitor across MAX30102 VCC and GND directly at the module pins.
- [ ] **Optical Window Integrity:** Inspect MAX30102 glass for dust, fingerprints, or resin flux residue before enclosure assembly.
