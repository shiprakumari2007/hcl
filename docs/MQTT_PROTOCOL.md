# MQTT Messaging Protocol Specification

**Project:** P_174 — Occupational Safety & Health Monitoring System  
**Document Ref:** `docs/MQTT_PROTOCOL.md`  
**Compatibility:** HiveMQ Cloud, HiveMQ Community Edition, Eclipse Mosquitto, EMQX

---

## 1. Topic Hierarchy Architecture

All topics follow the root namespace `safetywearable/` followed by the unique hardware `deviceId`:

```text
safetywearable/
  └── {deviceId}/
        ├── telemetry   (Periodic vital signs publication)
        ├── status      (Device connectivity & Last Will Testament)
        ├── alert       (Edge-detected critical alerts)
        └── command     (Inbound commands from backend to wearable)
```

---

## 2. Topic Definitions & Payload Contracts

### 2.1 Telemetry Channel
* **Topic:** `safetywearable/{deviceId}/telemetry`
* **Direction:** ESP32 Node ──► MQTT Broker ──► Backend
* **QoS Level:** `0` (At most once, non-blocking edge transmit)
* **Retained:** `false`
* **Publish Interval:** 5000 ms (5 seconds)

#### JSON Payload Schema:
```json
{
  "deviceId": "SW-001",
  "heartRate": 94,
  "heartRateValid": true,
  "spo2": 97,
  "spo2Valid": true,
  "temperature": 37.2,
  "temperatureValid": true,
  "battery": 82,
  "signalQuality": "VALID",
  "timestamp": "2026-09-30T09:12:00Z"
}
```

#### Field Specifications:
| Field | Type | Required | Valid Range / Values | Description |
| :--- | :--- | :--- | :--- | :--- |
| `deviceId` | String | **Yes** | Alphanumeric (e.g., `SW-001`) | Unique identifier programmed in device firmware |
| `heartRate` | Integer / Null | **Yes** | `40` – `220` or `null` | Instantaneous heart rate in beats per minute |
| `heartRateValid` | Boolean | **Yes** | `true` / `false` | High-confidence peak detection flag |
| `spo2` | Integer / Null | **Yes** | `70` – `100` or `null` | Arterial blood oxygen saturation percentage |
| `spo2Valid` | Boolean | **Yes** | `true` / `false` | High-confidence R-ratio algorithm flag |
| `temperature` | Float / Null | **Yes** | `20.0` – `45.0` or `null` | Non-contact skin/object temperature in °C |
| `temperatureValid`| Boolean | **Yes** | `true` / `false` | MLX90614 sensor communication status |
| `battery` | Integer / Null | No | `0` – `100` or `null` | Battery percentage if fuel gauge is present |
| `signalQuality` | String | **Yes** | `VALID`, `NO_CONTACT`, `LOW_SIGNAL`, `MOTION_DEGRADED` | Optical PPG quality assessment |
| `timestamp` | String | No | ISO 8601 UTC | Edge timestamp (Backend will fallback to server time) |

> [!IMPORTANT]
> **No Fake Zero Values:** When a worker removes the wearable or skin contact is lost, `heartRate` and `spo2` MUST be sent as JSON `null` (not `0`), with `heartRateValid: false` and `signalQuality: "NO_CONTACT"`.

---

### 2.2 Status & Lifecycle Channel (LWT)
* **Topic:** `safetywearable/{deviceId}/status`
* **Direction:** ESP32 Node ──► MQTT Broker ──► Backend
* **QoS Level:** `1` (At least once)
* **Retained:** `true`

#### Last Will and Testament (LWT) Payload:
Configured at MQTT connection initiation. Dispatched by the broker if the TCP socket dies unexpectedly:
```json
{
  "deviceId": "SW-001",
  "status": "OFFLINE",
  "reason": "CONNECTION_LOST"
}
```

#### Online Lifecycle Payload:
Dispatched by the device immediately after successful MQTT handshake:
```json
{
  "deviceId": "SW-001",
  "status": "ONLINE",
  "firmwareVersion": "3.0.0",
  "ipAddress": "192.168.1.145",
  "rssi": -62
}
```

---

### 2.3 Edge Alert Channel
* **Topic:** `safetywearable/{deviceId}/alert`
* **Direction:** ESP32 Node ──► MQTT Broker ──► Backend
* **QoS Level:** `1` (Guaranteed delivery)
* **Retained:** `false`

#### JSON Payload Schema:
```json
{
  "deviceId": "SW-001",
  "alertType": "HIGH_HEART_RATE",
  "severity": "WARNING",
  "value": 116.0,
  "threshold": 110.0,
  "message": "Worker Heart Rate exceeded safety threshold",
  "timestamp": "2026-09-30T09:12:05Z"
}
```

#### Allowed `alertType` Enumerations:
* `HIGH_HEART_RATE`: Worker heart rate $> 110\text{ BPM}$ (Exertion / Heat Exhaustion).
* `LOW_SPO2`: SpO2 $< 92\%$ (Hypoxia / Respiratory distress).
* `HIGH_TEMPERATURE`: Skin/Body temperature $> 38.0^\circ\text{C}$ (Hyperthermia / Heat Stroke).
* `SENSOR_ERROR`: I2C bus error or detached optical module.
* `DEVICE_OFFLINE`: Watchdog timeout.

#### Allowed `severity` Enumerations:
* `INFO`
* `WARNING`
* `CRITICAL`
