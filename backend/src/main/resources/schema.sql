-- ============================================================================
-- PostgreSQL Schema for P_174: Occupational Safety & Health Monitoring System
-- ============================================================================

CREATE TABLE IF NOT EXISTS devices (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL UNIQUE,
    name VARCHAR(128) NOT NULL,
    status VARCHAR(32) NOT NULL DEFAULT 'OFFLINE',
    battery INTEGER,
    firmware_version VARCHAR(32),
    last_seen TIMESTAMP WITH TIME ZONE,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_devices_status ON devices (status);
CREATE INDEX IF NOT EXISTS idx_devices_last_seen ON devices (last_seen DESC);

CREATE TABLE IF NOT EXISTS workers (
    id BIGSERIAL PRIMARY KEY,
    worker_code VARCHAR(64) NOT NULL UNIQUE,
    name VARCHAR(128) NOT NULL,
    site VARCHAR(128) NOT NULL,
    role VARCHAR(64) DEFAULT 'Construction Worker',
    device_id VARCHAR(64) REFERENCES devices(device_id) ON DELETE SET NULL,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_workers_device_id ON workers (device_id);
CREATE INDEX IF NOT EXISTS idx_workers_site ON workers (site);

CREATE TABLE IF NOT EXISTS health_readings (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL REFERENCES devices(device_id) ON DELETE CASCADE,
    timestamp TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    heart_rate INTEGER,
    heart_rate_valid BOOLEAN NOT NULL DEFAULT FALSE,
    spo2 INTEGER,
    spo2_valid BOOLEAN NOT NULL DEFAULT FALSE,
    temperature DOUBLE PRECISION,
    temperature_valid BOOLEAN NOT NULL DEFAULT FALSE,
    battery INTEGER,
    signal_quality VARCHAR(32) NOT NULL DEFAULT 'VALID'
);

CREATE INDEX IF NOT EXISTS idx_readings_device_time ON health_readings (device_id, timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_readings_timestamp ON health_readings (timestamp DESC);

CREATE TABLE IF NOT EXISTS alerts (
    id BIGSERIAL PRIMARY KEY,
    device_id VARCHAR(64) NOT NULL REFERENCES devices(device_id) ON DELETE CASCADE,
    worker_id BIGINT REFERENCES workers(id) ON DELETE SET NULL,
    alert_type VARCHAR(64) NOT NULL,
    severity VARCHAR(32) NOT NULL DEFAULT 'WARNING',
    value DOUBLE PRECISION,
    threshold DOUBLE PRECISION,
    message TEXT NOT NULL,
    timestamp TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    acknowledged BOOLEAN NOT NULL DEFAULT FALSE,
    acknowledged_at TIMESTAMP WITH TIME ZONE,
    acknowledged_by VARCHAR(128)
);

CREATE INDEX IF NOT EXISTS idx_alerts_device_id ON alerts (device_id);
CREATE INDEX IF NOT EXISTS idx_alerts_timestamp ON alerts (timestamp DESC);
CREATE INDEX IF NOT EXISTS idx_alerts_acknowledged ON alerts (acknowledged);
