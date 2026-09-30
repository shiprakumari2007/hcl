/**
 * ============================================================================
 * P_174 Standalone Test Backend & MQTT Bridge (Zero External Dependencies)
 * ============================================================================
 * Implements the exact same REST APIs, SSE streaming, and 15s Debounce Alert
 * Engine as the Java Spring Boot service. Used for local end-to-end execution
 * and testing without requiring external PostgreSQL, Maven, or Docker installations.
 * ============================================================================
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const net = require('net');

const HTTP_PORT = process.env.PORT || 8080;
const MQTT_PORT = 1883;

// ── In-Memory Database State (Mirroring PostgreSQL schema) ───────────────────
let devices = [
  { id: 1, deviceId: 'SW-001', name: 'ESP32 Wearable Unit 1', status: 'ONLINE', battery: 85, firmwareVersion: '4.0.0-MQTT', lastSeen: new Date().toISOString() },
  { id: 2, deviceId: 'SW-002', name: 'ESP32 Wearable Unit 2', status: 'ONLINE', battery: 92, firmwareVersion: '4.0.0-MQTT', lastSeen: new Date().toISOString() },
  { id: 3, deviceId: 'SW-003', name: 'ESP32 Wearable Unit 3', status: 'OFFLINE', battery: 74, firmwareVersion: '4.0.0-MQTT', lastSeen: new Date(Date.now() - 7200000).toISOString() },
  { id: 4, deviceId: 'SW-TEST-001', name: 'Mock Simulator Unit Alpha', status: 'ONLINE', battery: 95, firmwareVersion: '4.0.0-SIM', lastSeen: new Date().toISOString() }
];

let workers = [
  { id: 1, workerCode: 'W-101', name: 'Rajesh Kumar', site: 'Tower A - 12th Floor Rebar', role: 'Steel Fixer', deviceId: 'SW-001' },
  { id: 2, workerCode: 'W-102', name: 'Amit Sharma', site: 'Sub-Structure Concrete Pour', role: 'Concrete Vibrator Op', deviceId: 'SW-002' },
  { id: 3, workerCode: 'W-103', name: 'Suresh Patel', site: 'Sector 62 Excavation Pit', role: 'Excavation Foreman', deviceId: 'SW-003' },
  { id: 4, workerCode: 'W-104', name: 'Vikram Singh', site: 'Tower B Scaffolding', role: 'Rigger / Scaffolder', deviceId: 'SW-TEST-001' }
];

let healthReadings = [
  { id: 1, deviceId: 'SW-001', timestamp: new Date(Date.now() - 10000).toISOString(), heartRate: 88, heartRateValid: true, spo2: 98, spo2Valid: true, temperature: 37.1, temperatureValid: true, battery: 85, signalQuality: 'VALID' },
  { id: 2, deviceId: 'SW-002', timestamp: new Date(Date.now() - 10000).toISOString(), heartRate: 94, heartRateValid: true, spo2: 97, spo2Valid: true, temperature: 37.3, temperatureValid: true, battery: 92, signalQuality: 'VALID' }
];

let alerts = [
  {
    id: 1,
    deviceId: 'SW-002',
    workerId: 2,
    workerName: 'Amit Sharma',
    workerCode: 'W-102',
    alertType: 'HIGH_TEMPERATURE',
    severity: 'WARNING',
    value: 38.3,
    threshold: 38.0,
    message: 'Worker skin temperature measured at 38.3°C indicating heat stress risk',
    timestamp: new Date(Date.now() - 300000).toISOString(),
    acknowledged: false,
    acknowledgedAt: null,
    acknowledgedBy: null
  }
];

// Alert Engine Tracking
const abnormalTracker = new Map(); // key -> firstDetectedTime
const cooldownTracker = new Map(); // key -> lastAlertTime
const ALERT_PERSISTENCE_MS = 15000;
const ALERT_COOLDOWN_MS = 60000;

// SSE Subscribers
const sseClients = new Set();

function broadcastSSE(event, data) {
  const payload = `event: ${event}\ndata: ${JSON.stringify(data)}\n\n`;
  for (const client of sseClients) {
    try {
      client.write(payload);
    } catch (e) {
      sseClients.delete(client);
    }
  }
}

// ── Ingest Telemetry & Evaluate Alerts ───────────────────────────────────────
function ingestTelemetry(telemetry) {
  const deviceId = telemetry.deviceId;
  if (!deviceId) return;

  const now = new Date();

  // 1. Update Device lastSeen and battery
  let device = devices.find(d => d.deviceId === deviceId);
  if (!device) {
    device = {
      id: devices.length + 1,
      deviceId: deviceId,
      name: `Device ${deviceId}`,
      status: 'ONLINE',
      battery: telemetry.battery,
      firmwareVersion: '4.0.0-MQTT',
      lastSeen: now.toISOString()
    };
    devices.push(device);
  } else {
    device.status = 'ONLINE';
    device.lastSeen = now.toISOString();
    if (telemetry.battery != null) device.battery = telemetry.battery;
  }

  // 2. Persist Reading
  const reading = {
    id: healthReadings.length + 1,
    deviceId: deviceId,
    timestamp: telemetry.timestamp || now.toISOString(),
    heartRate: telemetry.heartRate,
    heartRateValid: !!telemetry.heartRateValid,
    spo2: telemetry.spo2,
    spo2Valid: !!telemetry.spo2Valid,
    temperature: telemetry.temperature,
    temperatureValid: !!telemetry.temperatureValid,
    battery: telemetry.battery,
    signalQuality: telemetry.signalQuality || 'VALID'
  };
  healthReadings.push(reading);

  // Keep max 500 readings in memory
  if (healthReadings.length > 500) healthReadings.shift();

  // Broadcast to SSE
  broadcastSSE('telemetry', telemetry);

  // 3. Alert Engine Evaluation
  evaluateAlerts(telemetry, now.getTime());
}

function evaluateAlerts(t, nowMs) {
  const deviceId = t.deviceId;

  const checks = [
    {
      type: 'HIGH_HEART_RATE',
      active: t.heartRateValid && t.heartRate != null && t.heartRate > 110,
      val: t.heartRate,
      thresh: 110,
      severity: 'WARNING',
      msg: `Persistent elevated Heart Rate (${t.heartRate} bpm) exceeding threshold (110 bpm)`
    },
    {
      type: 'LOW_SPO2',
      active: t.spo2Valid && t.spo2 != null && t.spo2 < 92,
      val: t.spo2,
      thresh: 92,
      severity: 'CRITICAL',
      msg: `Arterial blood oxygen saturation dropped to ${t.spo2}% (threshold: 92%)`
    },
    {
      type: 'HIGH_TEMPERATURE',
      active: t.temperatureValid && t.temperature != null && t.temperature > 38.0,
      val: t.temperature,
      thresh: 38.0,
      severity: 'WARNING',
      msg: `Body temperature measured at ${t.temperature}°C indicating heat stress risk`
    }
  ];

  for (const c of checks) {
    const key = `${deviceId}:${c.type}`;
    if (c.active) {
      if (!abnormalTracker.has(key)) {
        abnormalTracker.set(key, nowMs);
      }
      const firstSeen = abnormalTracker.get(key);
      if (nowMs - firstSeen >= ALERT_PERSISTENCE_MS) {
        const lastAlert = cooldownTracker.get(key) || 0;
        if (nowMs - lastAlert >= ALERT_COOLDOWN_MS) {
          cooldownTracker.set(key, nowMs);

          // Find worker
          const worker = workers.find(w => w.deviceId === deviceId);

          const newAlert = {
            id: alerts.length + 1,
            deviceId: deviceId,
            workerId: worker ? worker.id : null,
            workerName: worker ? worker.name : null,
            workerCode: worker ? worker.workerCode : null,
            alertType: c.type,
            severity: c.severity,
            value: c.val,
            threshold: c.thresh,
            message: c.msg,
            timestamp: new Date().toISOString(),
            acknowledged: false,
            acknowledgedAt: null,
            acknowledgedBy: null
          };

          alerts.unshift(newAlert);
          console.log(`[ALERT GENERATED] ${c.type} on ${deviceId} (Value: ${c.val})`);
          broadcastSSE('alert', newAlert);
        }
      }
    } else {
      abnormalTracker.delete(key);
    }
  }
}

// ── Device Watchdog (Checks for inactive devices every 10s) ──────────────────
setInterval(() => {
  const cutoff = Date.now() - 30000;
  for (const d of devices) {
    if (d.status === 'ONLINE') {
      const lastTime = new Date(d.lastSeen).getTime();
      if (lastTime < cutoff) {
        d.status = 'OFFLINE';
        console.log(`[WATCHDOG] Device ${d.deviceId} marked OFFLINE (timeout > 30s)`);
        broadcastSSE('status', { deviceId: d.deviceId, status: 'OFFLINE' });
      }
    }
  }
}, 10000);

// ── HTTP API Server ──────────────────────────────────────────────────────────
const server = http.createServer((req, res) => {
  // CORS Headers
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  if (req.method === 'OPTIONS') {
    res.writeHead(204);
    return res.end();
  }

  const parsedUrl = new URL(req.url, `http://${req.headers.host}`);
  const pathname = parsedUrl.pathname;

  // 1. SSE Stream
  if (pathname === '/api/stream/telemetry') {
    res.writeHead(200, {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache',
      'Connection': 'keep-alive'
    });
    res.write('event: connected\ndata: "SSE Stream Active"\n\n');
    sseClients.add(res);

    req.on('close', () => sseClients.delete(res));
    return;
  }

  // 2. Dashboard Summary
  if (pathname === '/api/dashboard/summary' && req.method === 'GET') {
    const online = devices.filter(d => d.status === 'ONLINE').length;
    const offline = devices.filter(d => d.status === 'OFFLINE').length;
    const activeAlerts = alerts.filter(a => !a.acknowledged).length;

    // Count warnings
    let warningCount = 0;
    for (const w of workers) {
      const latest = healthReadings.slice().reverse().find(r => r.deviceId === w.deviceId);
      if (latest) {
        if ((latest.heartRateValid && latest.heartRate > 110) ||
            (latest.spo2Valid && latest.spo2 < 92) ||
            (latest.temperatureValid && latest.temperature > 38.0)) {
          warningCount++;
        }
      }
    }

    const summary = {
      totalWorkers: workers.length,
      onlineWorkers: online,
      offlineWorkers: offline,
      activeAlerts: activeAlerts,
      normalWorkers: Math.max(0, workers.length - warningCount),
      warningWorkers: warningCount,
      siteName: 'Civil Site Infrastructure - Sector 62'
    };

    res.writeHead(200, { 'Content-Type': 'application/json' });
    return res.end(JSON.stringify(summary));
  }

  // 3. Workers List with Live Vitals
  if (pathname === '/api/workers' && req.method === 'GET') {
    const result = workers.map(w => {
      const dev = devices.find(d => d.deviceId === w.deviceId);
      const latest = healthReadings.slice().reverse().find(r => r.deviceId === w.deviceId);

      let safetyStatus = 'NORMAL';
      if (latest) {
        if ((latest.heartRateValid && latest.heartRate > 110) ||
            (latest.spo2Valid && latest.spo2 < 92) ||
            (latest.temperatureValid && latest.temperature > 38.0)) {
          safetyStatus = 'WARNING';
        }
      }

      return {
        id: w.id,
        workerCode: w.workerCode,
        name: w.name,
        site: w.site,
        role: w.role,
        deviceId: w.deviceId,
        deviceStatus: dev ? dev.status : 'OFFLINE',
        currentHeartRate: latest ? latest.heartRate : null,
        heartRateValid: latest ? latest.heartRateValid : false,
        currentSpo2: latest ? latest.spo2 : null,
        spo2Valid: latest ? latest.spo2Valid : false,
        currentTemperature: latest ? latest.temperature : null,
        temperatureValid: latest ? latest.temperatureValid : false,
        battery: dev ? dev.battery : null,
        signalQuality: latest ? latest.signalQuality : 'NO_CONTACT',
        safetyStatus: safetyStatus,
        lastUpdated: latest ? latest.timestamp : dev ? dev.lastSeen : null
      };
    });

    res.writeHead(200, { 'Content-Type': 'application/json' });
    return res.end(JSON.stringify(result));
  }

  // 4. Devices List
  if (pathname === '/api/devices' && req.method === 'GET') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    return res.end(JSON.stringify(devices));
  }

  // 5. Readings for Device
  if (pathname.startsWith('/api/readings/') && req.method === 'GET') {
    const parts = pathname.split('/');
    const devId = parts[3];
    const devReadings = healthReadings.filter(r => r.deviceId === devId).slice(-100);
    res.writeHead(200, { 'Content-Type': 'application/json' });
    return res.end(JSON.stringify(devReadings));
  }

  // 6. Alerts
  if (pathname === '/api/alerts' && req.method === 'GET') {
    const ackParam = parsedUrl.searchParams.get('acknowledged');
    let list = alerts;
    if (ackParam !== null) {
      const isAck = ackParam === 'true';
      list = alerts.filter(a => a.acknowledged === isAck);
    }
    res.writeHead(200, { 'Content-Type': 'application/json' });
    return res.end(JSON.stringify(list));
  }

  // 7. Acknowledge Alert
  if (pathname.startsWith('/api/alerts/') && pathname.endsWith('/acknowledge') && req.method === 'POST') {
    const parts = pathname.split('/');
    const alertId = parseInt(parts[3], 10);
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', () => {
      let data = {};
      try { data = JSON.parse(body); } catch (e) {}
      const alert = alerts.find(a => a.id === alertId);
      if (alert) {
        alert.acknowledged = true;
        alert.acknowledgedAt = new Date().toISOString();
        alert.acknowledgedBy = data.supervisor || 'Site Supervisor';
        res.writeHead(200, { 'Content-Type': 'application/json' });
        return res.end(JSON.stringify(alert));
      }
      res.writeHead(404, { 'Content-Type': 'application/json' });
      return res.end(JSON.stringify({ error: 'Alert not found' }));
    });
    return;
  }

  // 8. Direct Telemetry Ingest via HTTP POST (for testing & mock publisher)
  if (pathname === '/api/telemetry/ingest' && req.method === 'POST') {
    let body = '';
    req.on('data', chunk => body += chunk);
    req.on('end', () => {
      try {
        const payload = JSON.parse(body);
        ingestTelemetry(payload);
        res.writeHead(200, { 'Content-Type': 'application/json' });
        return res.end(JSON.stringify({ status: 'ACCEPTED', deviceId: payload.deviceId }));
      } catch (err) {
        res.writeHead(400, { 'Content-Type': 'application/json' });
        return res.end(JSON.stringify({ error: 'Invalid JSON' }));
      }
    });
    return;
  }

  // Fallback: Serve static Frontend build from frontend/dist
  const distPath = path.join(__dirname, '../frontend/dist');
  let filePath = path.join(distPath, pathname === '/' ? 'index.html' : pathname);

  if (fs.existsSync(filePath) && fs.statSync(filePath).isFile()) {
    const ext = path.extname(filePath).toLowerCase();
    const mimeTypes = {
      '.html': 'text/html',
      '.js': 'text/javascript',
      '.css': 'text/css',
      '.svg': 'image/svg+xml',
      '.json': 'application/json'
    };
    const contentType = mimeTypes[ext] || 'application/octet-stream';
    res.writeHead(200, { 'Content-Type': contentType });
    return fs.createReadStream(filePath).pipe(res);
  } else if (fs.existsSync(path.join(distPath, 'index.html'))) {
    res.writeHead(200, { 'Content-Type': 'text/html' });
    return fs.createReadStream(path.join(distPath, 'index.html')).pipe(res);
  }

  res.writeHead(404, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({ error: 'Not Found' }));
});

// Start HTTP Server
server.listen(HTTP_PORT, () => {
  console.log(`========================================================`);
  console.log(` P_174 Occupational Safety Backend & Web Interface`);
  console.log(` HTTP & SSE API: http://localhost:${HTTP_PORT}/api`);
  console.log(` Web Dashboard: http://localhost:${HTTP_PORT}/`);
  console.log(`========================================================`);
});

module.exports = { ingestTelemetry, server };
