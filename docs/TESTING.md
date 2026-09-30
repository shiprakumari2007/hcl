# Testing Strategy & Verification Protocol

**Project:** P_174 — Occupational Safety & Health Monitoring System  
**Document Ref:** `docs/TESTING.md`

---

## 1. Testing Tier Matrix

| Test Level | Scope | Method / Tool | Verification Target |
| :--- | :--- | :--- | :--- |
| **Unit Testing** | Backend Alert Engine & DTOs | JUnit 5 + Mockito | Threshold evaluation, 15s persistence debounce, 60s cooldown, invalid payload rejection. |
| **Integration Testing**| MQTT Ingestion to DB | Spring Boot Test + H2/Postgres | MQTT message deserialization, foreign key integrity, repository queries. |
| **End-to-End Simulation**| Wearable ──► Broker ──► Backend ──► UI | Python/Node Mock Publisher | Ingests mock telemetry (`SW-TEST-001`), asserts UI card update and alert generation. |
| **Offline Recovery** | Watchdog & Reconnection | Network disconnection test | ESP32 non-blocking loop resilience when broker drops; backend marks device `OFFLINE` after 30s. |
| **Physical Hardware** | Optical & Skin Temp accuracy | **Pending Physical Hardware** | Comparison against CE-certified finger pulse oximeter and tympanic thermometer. |

---

## 2. Automated Test Cases

### 2.1 Backend Alert Evaluation Tests
* `testNormalVitals_NoAlertGenerated`: HR=85, SpO2=98%, Temp=37.0°C. Asserts no alert is recorded.
* `testElevatedHeartRate_SingleReading_SuppressedByDebounce`: Single reading of HR=118 BPM. Asserts alert is NOT fired before the 15-second persistence window matures.
* `testElevatedHeartRate_Persistent_AlertFired`: HR=118 BPM sustained for $> 15\text{ seconds}$. Asserts `HIGH_HEART_RATE` alert is inserted into database and broadcasted.
* `testAlertCooldownEnforcement`: High vitals sustained for 120 seconds. Asserts only 2 alerts are logged (1 initial + 1 after 60s cooldown), avoiding spam.
* `testInvalidSensorSignal_NoFalseAlert`: HR=0 or null with `heartRateValid: false`. Asserts system registers `NO_CONTACT` and suppresses alert generation.

### 2.2 Device Watchdog Timeout Test
* Device `SW-001` sends telemetry, status set to `ONLINE`.
* Telemetry ceases for 35 seconds.
* Background scheduled task triggers: verifies status updates to `OFFLINE` in database and triggers SSE update.

---

## 3. Physical Hardware Boundary Disclosure

> [!WARNING]
> While all digital interfaces, protocols, and database queries are rigorously testable via simulation, **physical sensor SNR, skin contact impedance, sweat corrosion resistance, and optical sunlight saturation cannot be verified without physical hardware testing on-site.**
