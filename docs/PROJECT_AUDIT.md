# Project Audit: P_174 — Occupational Safety & Health Monitoring System

**Audit Date:** 2026-09-30  
**Project Identifier:** P_174 (formerly Patient Health Monitoring System / Occupational Safety Wearable)  
**Target Environment:** Civil construction safety monitoring (vitals, heat stress, exhaustion, hypoxia)

---

## 1. Existing Components Assessment

| Component | Current State in Workspace | Technical Assessment |
| :--- | :--- | :--- |
| **ESP32 Firmware** | Implemented in `SafetyWearable_Phase2/` (offline) and `SafetyWearable_Phase3/` (Blynk). | Well-structured non-blocking `millis()` loop with rolling FIFO for MAX30102. However, Phase 3 relies strictly on the proprietary Blynk cloud protocol (`BlynkSimpleEsp32.h`). |
| **MAX30102 (HR / SpO2)** | Implemented using SparkFun library + Maxim reference algorithm (`spo2_algorithm.h`). | Functional sliding window (100 buffer length, 25 shift), but lacks granular signal quality states (e.g., `NO_CONTACT`, `LOW_SIGNAL`, `MOTION_DEGRADED`) and sends `0` or freezes instead of reporting `null` / invalid state. |
| **MLX90614 (Body Temp)** | Implemented using Adafruit library over I2C at `0x5A`. | Clean, sampled every 2000ms. However, assumes skin temperature equals core body temperature without thermal offset or housing heat isolation. |
| **0.96" SSD1306 OLED** | Implemented using Adafruit SSD1306 + GFX at `0x3C`. | Excellent visual layout and custom degree symbol routine. Needs status indicators for Wi-Fi and MQTT connectivity. |
| **Blynk 2.0 Integration** | Implemented in `SafetyWearable_Phase3/SafetyWearable_Phase3.ino`. | Works for prototype mobile app notifications, but creates vendor lock-in, lacks enterprise multi-tenant database persistence, and cannot integrate with industrial web dashboards. |
| **Backend** | **Missing** | No centralized backend service exists in the repository. |
| **Database** | **Missing** | No persistent time-series or relational database exists. |
| **Frontend** | **Missing** | No enterprise web interface or supervisor dashboard exists. |
| **MQTT Integration** | **Missing** | No standard broker publishing or topic hierarchy exists. |
| **Testing Suite** | **Missing** | No automated unit, integration, or end-to-end tests exist. |
| **Documentation** | Minimal inline comments in sketches. | Lacks architecture specifications, API documentation, hardware schematics, and protocol contracts. |

---

## 2. Inventory of Existing Files

| File Path | Size | Purpose & Evaluation |
| :--- | :--- | :--- |
| `SafetyWearable_Phase2/SafetyWearable_Phase2.ino` | ~35.7 KB | Complete offline Arduino sketch. Reads MAX30102, MLX90614, renders to SSD1306 via non-blocking `millis()`. **Status: Preserved as offline baseline.** |
| `SafetyWearable_Phase3/SafetyWearable_Phase3.ino` | ~25.3 KB | Online Arduino sketch adding WiFi and Blynk 2.0. Telemetry pushed every 5s, alerts triggered via `Blynk.logEvent`. **Status: Preserved as legacy test mode.** |

---

## 3. Existing Architecture

```
[ Worker Wearable: ESP32 ]
    ├── MAX30102 (I2C 0x57)
    ├── MLX90614 (I2C 0x5A)
    └── SSD1306 OLED (I2C 0x3C)
         │
    (Proprietary Blynk Protocol over TCP/SSL)
         │
         ▼
[ Blynk Cloud (Third-Party SaaS) ]
         │
         ▼
[ Blynk Mobile App ]
```

---

## 4. Problems Identified

1. **Vendor Lock-in & Architecture Inflexibility:**
   - The system is tied to Blynk. It cannot store historical vitals in an internal PostgreSQL database or serve an on-premise industrial dashboard.
2. **Missing Backend & Data Storage:**
   - No REST API or time-series storage exists for longitudinal safety audits, compliance reporting, or shift incident replay.
3. **Missing Device Identity Architecture:**
   - No explicit decoupling between Device ID (e.g., `SW-001`) and Worker ID (e.g., `W-104`).
4. **Data Integrity & Falsification Risk:**
   - When a sensor is disconnected or noisy, raw values risk defaulting to zero rather than explicitly reporting `null` / `valid: false`.
5. **Single-Point Threshold Alerting:**
   - Fixed constants (`HR > 110`, `Temp > 38.0°C`) are hardcoded into firmware, requiring physical reflashing to adjust safety guidelines.
6. **Lack of Ingress & Hardware Validation:**
   - Previous guides assumed the ESP32 VIN pin could directly accept a 3.7V LiPo without considering onboard LDO dropout limits (~1.1V for AMS1117).

---

## 5. Migration Plan (Keep, Modify, Replace, Add)

```
KEEP:
  - Phase 2 & Phase 3 code in their respective directories for backward compatibility and offline debugging.
  - Core MAX30102 sliding-window buffer acquisition logic and SparkFun/Maxim algorithm integration.
  - SSD1306 display formatting and non-blocking multi-rate scheduling architecture.

MODIFY:
  - Refactor ESP32 firmware to introduce SafetyWearable_MQTT:
    * Standard HiveMQ/Mosquitto compatible MQTT client (PubSubClient).
    * Structured JSON payloads with device identity, sensor validity flags, and signal quality metrics.
    * OLED display updated to show Wi-Fi, MQTT broker status, and data validity.

REPLACE:
  - Replace Blynk as the central system backbone with standard MQTT messaging.
  - Replace hardcoded device alerts with backend-managed alert evaluation and configurable persistence windows.

ADD:
  - Java Spring Boot 3 Backend:
    * MQTT Subscriber service consuming telemetry and alert topics.
    * PostgreSQL persistence (JPA entities: Device, Worker, HealthReading, Alert).
    * Alert Engine with debouncing (15s confirmation window) and cooldown (60s).
    * Comprehensive REST API (`/api/devices`, `/api/workers`, `/api/readings`, `/api/alerts`, `/api/dashboard/summary`).
    * Real-time WebSocket / SSE telemetry streaming.
  - React 18+ Supervisor Web Application:
    * Executive Safety Dashboard (total workers, online status, active hazard alerts).
    * Worker Monitoring Grid with live biometric status cards.
    * Worker Details with time-series charts (Heart Rate, SpO2, Skin Temp) and alert timelines.
    * Alert management with one-click supervisor acknowledgment.
    * Device inventory and online/offline tracking.
  - Mock Telemetry Publisher:
    * Python/Node script to simulate field devices (`SW-TEST-001`, `SW-TEST-002`) for testing without hardware.
  - Complete Engineering Documentation in `docs/`.
```
