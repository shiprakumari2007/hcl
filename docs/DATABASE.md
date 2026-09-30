# PostgreSQL Database Schema Specification

**Project:** P_174 — Occupational Safety & Health Monitoring System  
**Document Ref:** `docs/DATABASE.md`  
**Target Engine:** PostgreSQL 14+

---

## 1. Entity Relationship Overview

The schema cleanly isolates physical hardware (`devices`) from human operators (`workers`), continuous physiological time-series (`health_readings`), and discrete safety incidents (`alerts`).

```text
 ┌──────────────────────┐             ┌──────────────────────┐
 │       devices        │             │       workers        │
 ├──────────────────────┤             ├──────────────────────┤
 │ id (PK)              │             │ id (PK)              │
 │ device_id (UNIQUE)   │◄──┐         │ worker_code (UNIQUE) │
 │ name                 │   │         │ name                 │
 │ status               │   │         │ site                 │
 │ last_seen            │   │         │ device_id (FK)───────┼───► devices.device_id
 │ created_at           │   │         │ created_at           │
 │ updated_at           │   │         └──────────────────────┘
 └──────────────────────┘   │
            ▲               │
            │               │
            ├───────────────┼─────────────────────────┐
            │               │                         │
 ┌──────────┴───────────┐   │              ┌──────────┴───────────┐
 │   health_readings    │   │              │        alerts        │
 ├──────────────────────┤   │              ├──────────────────────┤
 │ id (BIGSERIAL PK)    │   │              │ id (BIGSERIAL PK)    │
 │ device_id (FK)───────┼───┘              │ device_id (FK)───────┼───► devices.device_id
 │ timestamp            │                  │ worker_id (FK)───────┼───► workers.id
 │ heart_rate           │                  │ alert_type           │
 │ heart_rate_valid     │                  │ severity             │
 │ spo2                 │                  │ value                │
 │ spo2_valid           │                  │ threshold            │
 │ temperature          │                  │ message              │
 │ temperature_valid    │                  │ timestamp            │
 │ battery              │                  │ acknowledged         │
 │ signal_quality       │                  │ acknowledged_at      │
 └──────────────────────┘                  │ acknowledged_by      │
                                           └──────────────────────┘
```

---

## 2. Table DDL Definitions & Constraints

### 2.1 Table: `devices`
Tracks physical wearable nodes provisioned in the field:
```sql
CREATE TABLE IF NOT EXISTS devices (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL UNIQUE,
    name VARCHAR(128) NOT NULL,
    status VARCHAR(32) NOT NULL DEFAULT 'OFFLINE', -- 'ONLINE', 'OFFLINE', 'WARNING'
    battery INTEGER,                              -- 0 - 100 or NULL
    firmware_version VARCHAR(32),
    last_seen TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_devices_status ON devices (status);
CREATE INDEX idx_devices_last_seen ON devices (last_seen DESC);
```

### 2.2 Table: `workers`
Represents the human labor force equipped with wearables:
```sql
CREATE TABLE IF NOT EXISTS workers (
    id BIGSERIAL PRIMARY KEY,
    worker_code VARCHAR(64) NOT NULL UNIQUE,       -- e.g. 'W-101'
    name VARCHAR(128) NOT NULL,
    site VARCHAR(128) NOT NULL,                    -- e.g. 'Tower A - Rebar Section'
    role VARCHAR(64) DEFAULT 'Worker',
    device_id VARCHAR(64) REFERENCES devices(device_id) ON DELETE SET NULL,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_workers_device_id ON workers (device_id);
CREATE INDEX idx_workers_site ON workers (site);
```

### 2.3 Table: `health_readings` (Time-Series Table)
Stores incoming physiological telemetry points:
```sql
CREATE TABLE IF NOT EXISTS health_readings (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL REFERENCES devices(device_id) ON DELETE CASCADE,
    timestamp TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    heart_rate INTEGER,                            -- BPM or NULL
    heart_rate_valid BOOLEAN NOT NULL DEFAULT FALSE,
    spo2 INTEGER,                                  -- % or NULL
    spo2_valid BOOLEAN NOT NULL DEFAULT FALSE,
    temperature DOUBLE PRECISION,                  -- °C or NULL
    temperature_valid BOOLEAN NOT NULL DEFAULT FALSE,
    battery INTEGER,                               -- % or NULL
    signal_quality VARCHAR(32) NOT NULL DEFAULT 'VALID' -- 'VALID', 'NO_CONTACT', 'LOW_SIGNAL', 'MOTION_DEGRADED'
);

-- Crucial indexes for rapid time-series dashboard filtering:
CREATE INDEX idx_readings_device_time ON health_readings (device_id, timestamp DESC);
CREATE INDEX idx_readings_timestamp ON health_readings (timestamp DESC);
```

### 2.4 Table: `alerts`
Tracks discrete physiological hazard events detected by the system:
```sql
CREATE TABLE IF NOT EXISTS alerts (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL REFERENCES devices(device_id) ON DELETE CASCADE,
    worker_id BIGINT REFERENCES workers(id) ON DELETE SET NULL,
    alert_type VARCHAR(64) NOT NULL,               -- 'HIGH_HEART_RATE', 'LOW_SPO2', 'HIGH_TEMPERATURE', etc.
    severity VARCHAR(32) NOT NULL DEFAULT 'WARNING',-- 'INFO', 'WARNING', 'CRITICAL'
    value DOUBLE PRECISION,                        -- Measured value that tripped alarm
    threshold DOUBLE PRECISION,                    -- Configured safety threshold
    message TEXT NOT NULL,
    timestamp TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    acknowledged BOOLEAN NOT NULL DEFAULT FALSE,
    acknowledged_at TIMESTAMP WITH TIME ZONE,
    acknowledged_by VARCHAR(128)
);

CREATE INDEX idx_alerts_device_id ON alerts (device_id);
CREATE INDEX idx_alerts_timestamp ON alerts (timestamp DESC);
CREATE INDEX idx_alerts_acknowledged ON alerts (acknowledged);
```
