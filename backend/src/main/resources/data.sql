-- ============================================================================
-- Seed Data for Development & Testing
-- ============================================================================

-- Provision Initial Wearable Hardware Nodes
INSERT INTO devices (device_id, name, status, battery, firmware_version, last_seen, created_at, updated_at)
VALUES 
  ('SW-001', 'ESP32 Wearable Unit 1', 'ONLINE', 85, '4.0.0-MQTT', NOW(), NOW(), NOW()),
  ('SW-002', 'ESP32 Wearable Unit 2', 'ONLINE', 92, '4.0.0-MQTT', NOW(), NOW(), NOW()),
  ('SW-003', 'ESP32 Wearable Unit 3', 'OFFLINE', 74, '4.0.0-MQTT', NOW() - INTERVAL '2 hours', NOW(), NOW()),
  ('SW-TEST-001', 'Mock Simulator Unit Alpha', 'ONLINE', 95, '4.0.0-SIM', NOW(), NOW(), NOW())
ON CONFLICT (device_id) DO NOTHING;

-- Register Active Construction Workers & Assign Devices
INSERT INTO workers (worker_code, name, site, role, device_id, created_at)
VALUES
  ('W-101', 'Rajesh Kumar', 'Tower A - 12th Floor Rebar', 'Steel Fixer', 'SW-001', NOW()),
  ('W-102', 'Amit Sharma', 'Sub-Structure Concrete Pour', 'Concrete Vibrator Op', 'SW-002', NOW()),
  ('W-103', 'Suresh Patel', 'Sector 62 Excavation Pit', 'Excavation Foreman', 'SW-003', NOW()),
  ('W-104', 'Vikram Singh', 'Tower B Scaffolding', 'Rigger / Scaffolder', 'SW-TEST-001', NOW())
ON CONFLICT (worker_code) DO NOTHING;
