# Mac Test & Verification Report: P_174 Occupational Safety Wearable

**Verification Date:** 2026-10-01  
**Environment:** macOS 26.2 (Darwin arm64 / Apple Silicon)  
**Verification Scope:** Complete End-to-End Software Stack Verification (Mock Telemetry → Mosquitto MQTT → Spring Boot → PostgreSQL → REST/SSE → React Frontend)

---

## 1. Executive Verification Summary

| Layer / Subsystem | Status | Verification Detail |
|:---|:---|:---|
| **Database (PostgreSQL 18.6)** | **INTEGRATION TESTED** | `safety_wearable_db` created, tables initialized (`devices`, `workers`, `health_readings`, `alerts`), indexes verified, FK constraints validated, live inserts/queries confirmed. |
| **MQTT Broker (Mosquitto 2.0)** | **INTEGRATION TESTED** | Local broker running on port 1883. Pub/Sub verified via `mosquitto_sub`/`mosquitto_pub` and live Spring Boot ingestion. |
| **Backend (Spring Boot 3.2.4)** | **INTEGRATION TESTED** | Unit & integration tests executed (`mvn clean test`). Backend running on port 8080 with JPA, Paho MQTT, watchdog, and SSE streaming active. |
| **REST API Layer** | **INTEGRATION TESTED** | Verified endpoints: `/api/dashboard/summary`, `/api/workers`, `/api/devices`, `/api/readings/{deviceId}`, `/api/alerts`, `/api/alerts/{id}/acknowledge`. |
| **Real-time Streaming (SSE)** | **INTEGRATION TESTED** | `/api/stream/telemetry` connection verified; live MQTT ingestion broadcasts `telemetry`, `status`, and `alert` events to SSE clients. |
| **Alert Engine & Debounce** | **INTEGRATION TESTED** | 15s persistence window, 60s cooldown, invalid reading suppression (`NO_CONTACT`), and supervisor acknowledgment flow all tested with live MQTT payloads. |
| **Device Watchdog Service** | **INTEGRATION TESTED** | 30s inactivity timeout verified: devices automatically transition from `ONLINE` to `OFFLINE`. |
| **Frontend (React 18 + Vite 5)** | **INTEGRATION TESTED** | Clean build (`npm run build`), dev server on port 5173, Vite proxy forwarding to backend, dashboard metrics, worker cards, alert acknowledgment verified. |
| **Mock Telemetry Suite** | **SOFTWARE TESTED** | Python publisher tested across scenarios (`normal`, `heat_stress`, `hypoxia`, `no_contact`). |
| **Physical ESP32 & Sensors** | **PENDING HARDWARE TEST** | MAX30102, MLX90614, SSD1306, ESP32 Wi-Fi/LDO hardware testing remains pending physical hardware bench. |

---

## 2. Test Execution & Evidence

### 2.1 Database & Persistence
- Schema: 4 tables (`devices`, `workers`, `health_readings`, `alerts`) with B-Tree indexes on `timestamp DESC`, `device_id`, `status`, and `acknowledged`.
- Seed Data: 4 devices (`SW-001`, `SW-002`, `SW-003`, `SW-TEST-001`) and 4 workers (`W-101` to `W-104`).
- Persistence Verification: Direct SQL queries confirmed incoming MQTT messages insert rows into `health_readings` and update `last_seen` on `devices`.

### 2.2 MQTT Ingestion Pipeline
- Topic Pattern: `safetywearable/+/telemetry`, `safetywearable/+/status`, `safetywearable/+/alert`
- Ingestion Latency: Sub-second parse and database write.
- Reconnection Handling: Paho MQTT client configured with automatic reconnect.

### 2.3 Alert Scenarios Verified
1. **Normal Telemetry (HR: 76-85, SpO2: 97-98%, Temp: 36.8-37.0°C)**:
   - Result: Recorded to `health_readings`; NO alerts generated.
2. **Elevated Heart Rate (HR: 124 > 110 threshold)**:
   - Result: 15s debounce observed; generated `HIGH_HEART_RATE` (Warning).
3. **Hypoxia (SpO2: 89 < 92 threshold)**:
   - Result: 15s debounce observed; generated `LOW_SPO2` (Critical).
4. **Hyperthermia / Heat Stress (Temp: 38.8 > 38.0°C threshold)**:
   - Result: 15s debounce observed; generated `HIGH_TEMPERATURE` (Warning).
5. **Sensor Detachment (`heartRateValid: false`, `signalQuality: NO_CONTACT`)**:
   - Result: Correctly suppressed; zero false alerts generated.
6. **Alert Cooldown**:
   - Result: Repeated high HR messages within 60s suppressed duplicate alert creation.

### 2.4 Supervisor Acknowledgment Workflow
- Endpoint: `POST /api/alerts/{id}/acknowledge`
- Verified: Alert record updated with `acknowledged = true`, `acknowledged_by = "Site Supervisor"`, `acknowledged_at = NOW()`.

### 2.5 Real-Time SSE Stream & Watchdog
- Event Source: `GET /api/stream/telemetry`
- Events Streamed: `telemetry`, `alert`, `status`
- Watchdog Scan: Scheduled every 10s. When device inactivity > 30s, status transitioned `ONLINE → OFFLINE` in DB and broadcast via SSE.

### 2.6 Frontend Build & Dev Server
- Build: `npm run build` completed in 887ms producing optimized bundle (`dist/assets/index-*.js`).
- Dev Server: Running at `http://localhost:5173` with Vite proxy forwarding `/api` to `http://localhost:8080`.
- Real-time updates: Tested through proxy; summary cards and worker cards reflect live vitals and hazard alerts.

---

## 3. Issues Discovered & Remediation

| Issue | Root Cause | Fix Applied |
|:---|:---|:---|
| Maven Mockito test failure on Java 27 | ByteBuddy bundled with Spring Boot 3.2.4 lacks native Java 27 support | Added JVM property `-Dnet.bytebuddy.experimental=true` |
| Local DB authentication mismatch | `application.properties` default was `postgres/postgres` | Set default username to `shahabahmad` and injected runtime password via `SPRING_DATASOURCE_PASSWORD` env var |
| Missing `paho-mqtt` Python dependency | Python 3.14 environment lacked MQTT client library | Installed via `pip3 install paho-mqtt` |
| Mock publisher API deprecation | `paho-mqtt` 2.x deprecates `CallbackAPIVersion.VERSION1` default | Documented warning; publisher remains fully functional |

---

## 4. Hardware Boundary Disclosure

The following physical aspects remain **PENDING HARDWARE TEST**:
- Optical PPG signal-to-noise ratio under ambient sunlight and dark skin pigmentation.
- MLX90614 emissivity calibration and body thermal conduction through wearable housing.
- LiPo battery voltage dropout under ESP32 Wi-Fi transmission bursts (AMS1117 vs MCP1700 LDO).
- Continuous field testing under high ambient temperature and physical exertion.
