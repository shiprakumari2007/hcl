# REST & Streaming API Specification

**Project:** P_174 — Occupational Safety & Health Monitoring System  
**Document Ref:** `docs/API.md`  
**Base URL:** `http://localhost:8080/api`

---

## 1. Summary of Endpoints

| Method | Endpoint | Description |
| :--- | :--- | :--- |
| `GET` | `/dashboard/summary` | Aggregated site overview metrics (total, online, alerts, status breakdown) |
| `GET` | `/devices` | List all provisioned wearable devices and their connectivity state |
| `GET` | `/devices/{deviceId}` | Retrieve specific device profile and latest connection metadata |
| `GET` | `/workers` | List all registered construction workers and their linked wearable ID |
| `GET` | `/workers/{id}` | Worker detail profile, linked device, and current vital signs |
| `GET` | `/readings/{deviceId}` | Retrieve historical telemetry for a device (supports `?limit=100` and date filtering) |
| `GET` | `/alerts` | Query active and past safety alerts (supports `?acknowledged=false`) |
| `GET` | `/alerts/{deviceId}` | Retrieve alerts filtered by device ID |
| `POST` | `/alerts/{id}/acknowledge` | Mark an alert incident as acknowledged by supervisor |
| `GET` | `/stream/telemetry` | Server-Sent Events (SSE) stream for zero-latency live dashboard updates |

---

## 2. Endpoint Details & Schemas

### 2.1 Dashboard Summary
* **Endpoint:** `GET /api/dashboard/summary`
* **Response `200 OK`:**
```json
{
  "totalWorkers": 6,
  "onlineWorkers": 4,
  "offlineWorkers": 2,
  "activeAlerts": 1,
  "normalWorkers": 3,
  "warningWorkers": 1,
  "siteName": "Civil Infrastructure - Sector 62 Site"
}
```

---

### 2.2 Workers List & Real-time State
* **Endpoint:** `GET /api/workers`
* **Response `200 OK`:**
```json
[
  {
    "id": 1,
    "workerCode": "W-101",
    "name": "Rajesh Kumar",
    "site": "Tower B - 14th Floor Rebar",
    "role": "Steel Fixer",
    "deviceId": "SW-001",
    "deviceStatus": "ONLINE",
    "currentHeartRate": 104,
    "heartRateValid": true,
    "currentSpo2": 97,
    "spo2Valid": true,
    "currentTemperature": 37.6,
    "temperatureValid": true,
    "battery": 82,
    "signalQuality": "VALID",
    "safetyStatus": "NORMAL",
    "lastUpdated": "2026-09-30T09:12:05Z"
  }
]
```

---

### 2.3 Device Historical Telemetry
* **Endpoint:** `GET /api/readings/{deviceId}?from=2026-09-30T08:00:00Z&to=2026-09-30T10:00:00Z&limit=100`
* **Response `200 OK`:**
```json
[
  {
    "id": 501,
    "deviceId": "SW-001",
    "timestamp": "2026-09-30T09:12:00Z",
    "heartRate": 98,
    "heartRateValid": true,
    "spo2": 98,
    "spo2Valid": true,
    "temperature": 37.4,
    "temperatureValid": true,
    "battery": 82,
    "signalQuality": "VALID"
  }
]
```

---

### 2.4 Safety Alerts
* **Endpoint:** `GET /api/alerts?acknowledged=false`
* **Response `200 OK`:**
```json
[
  {
    "id": 42,
    "deviceId": "SW-001",
    "workerId": 1,
    "workerName": "Rajesh Kumar",
    "workerCode": "W-101",
    "alertType": "HIGH_HEART_RATE",
    "severity": "WARNING",
    "value": 118.0,
    "threshold": 110.0,
    "message": "Persistent elevated Heart Rate (118 bpm) exceeding threshold (110 bpm)",
    "timestamp": "2026-09-30T09:11:45Z",
    "acknowledged": false,
    "acknowledgedAt": null,
    "acknowledgedBy": null
  }
]
```

---

### 2.5 Acknowledge Alert
* **Endpoint:** `POST /api/alerts/{id}/acknowledge`
* **Request Body:**
```json
{
  "supervisor": "Safety Officer Singh",
  "note": "Worker advised to take 15 min water break in shaded zone."
}
```
* **Response `200 OK`:**
```json
{
  "id": 42,
  "acknowledged": true,
  "acknowledgedAt": "2026-09-30T09:13:00Z",
  "acknowledgedBy": "Safety Officer Singh"
}
```

---

### 2.6 Server-Sent Events (SSE) Live Feed
* **Endpoint:** `GET /api/stream/telemetry`
* **Headers:** `Accept: text/event-stream`
* **Stream Events:**
```text
event: telemetry
data: {"deviceId":"SW-001","heartRate":99,"spo2":97,"temperature":37.5,"status":"NORMAL","timestamp":"2026-09-30T09:12:10Z"}

event: alert
data: {"id":43,"deviceId":"SW-001","alertType":"HIGH_HEART_RATE","severity":"WARNING","value":115.0}
```
