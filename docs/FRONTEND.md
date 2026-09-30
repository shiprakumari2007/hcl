# Frontend Architecture & UI Specification

**Project:** P_174 — Occupational Safety & Health Monitoring System  
**Document Ref:** `docs/FRONTEND.md`  
**Framework:** React 18+ (Single Page Application with Modern CSS & Lucide Icons)

---

## 1. Design Philosophy & Purpose

The P_174 Frontend is engineered specifically as an **Industrial Site Supervisor Dashboard**. It provides site safety engineers with instantaneous situational awareness regarding physical heat stress, exhaustion, and hypoxia among workers operating in extreme outdoor environments.

### Core UX Principles:
1. **Zero Fake Metrics:** If a sensor signal is lost or motion-degraded, the UI clearly shows `No Signal` / `--` with amber/grey warning badges rather than displaying misleading zeros.
2. **Instant Status Triage:** Color-coded status hierarchy:
   * 🟢 **NORMAL**: All vitals within safe thresholds, device active.
   * 🟡 **WARNING**: One or more vitals approaching limit or persistent alert triggered.
   * 🔴 **CRITICAL**: Severe physiological anomaly (e.g., Temp $> 39^\circ\text{C}$ or SpO2 $< 90\%$).
   * ⚪ **OFFLINE**: No telemetry received within the 30-second watchdog window.
3. **No Polling Overhead:** Uses real-time Server-Sent Events (SSE) / WebSocket with fallback to efficient REST intervals.

---

## 2. Component Hierarchy

```text
src/
├── components/
│   ├── Navbar.jsx             (Top bar with site selector, active alert badge, live clock)
│   ├── SummaryCards.jsx       (KPI cards: Total Workers, Online, Active Alerts, Warnings)
│   ├── WorkerCard.jsx         (Individual worker live tile with HR, SpO2, Temp, Signal, Battery)
│   ├── AlertBanner.jsx        (Prominent sticky emergency banner when critical alerts exist)
│   ├── VitalsChart.jsx        (Interactive historical time-series chart with zoom presets)
│   ├── AlertTable.jsx         (Incident audit log with supervisor acknowledgment button)
│   └── DeviceTable.jsx        (Hardware inventory, firmware version, and battery health)
├── pages/
│   ├── DashboardPage.jsx      (Executive site overview & worker card grid)
│   ├── WorkersPage.jsx        (Comprehensive worker roster & vital signs monitoring)
│   ├── WorkerDetailPage.jsx    (Single worker deep-dive: trend graphs & alert history)
│   ├── AlertsPage.jsx         (All active and resolved safety alerts with ack workflow)
│   └── DevicesPage.jsx        (Hardware device provisioning and online/offline tracking)
├── services/
│   ├── api.js                 (REST client handling fetch requests to Spring Boot)
│   └── sse.js                 (Server-Sent Events connection manager with auto-reconnect)
└── styles/
    └── index.css              (Clean industrial dashboard theme with high-contrast accessibility)
```

---

## 3. Page Specifications

### 3.1 Executive Dashboard (`/`)
* **KPI Metrics:**
  * Active Workers Online (e.g. `4 / 6 Online`)
  * Active Hazard Alerts (e.g. `1 Pending Acknowledgment`)
  * Workers in Elevated Warning State (e.g. `1 Warning`)
  * Site Thermal Condition (Derived from ambient readings)
* **Live Worker Grid:** Fast visual scan of all workers with live pulse animation if heart rate is elevated.

### 3.2 Worker Detail Analytics (`/workers/:id`)
* Displays worker identity, current assigned wearable (`SW-001`), site location, and contact.
* Interactive time-series charts powered by SVG/Canvas rendering:
  * **Heart Rate Graph:** Safe baseline zone ($60\text{--}100\text{ BPM}$) vs Alert threshold line ($110\text{ BPM}$).
  * **Oxygen Saturation Graph:** Safe zone ($95\text{--}100\%$) vs Hypoxia threshold ($92\%$).
  * **Body Temperature Graph:** Normal zone ($36.5\text{--}37.5^\circ\text{C}$) vs Heat Stress threshold ($38.0^\circ\text{C}$).
* Time-range selectors: **Last 1 Hour**, **Last 6 Hours**, **Full Shift (24h)**.

### 3.3 Safety Alert Incident Log (`/alerts`)
* Full audit trail of every triggered threshold event.
* Supervisors can click **[Acknowledge]**, enter resolution notes (e.g., "Worker moved to hydration canopy"), and log their name.
