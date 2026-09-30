import React, { useEffect, useState } from 'react';
import VitalsChart from '../components/VitalsChart';
import AlertTable from '../components/AlertTable';
import { fetchReadings, fetchAlerts } from '../services/api';

export default function WorkerDetailPage({ worker, onBack, onAcknowledgeClick }) {
  const [readings, setReadings] = useState([]);
  const [alerts, setAlerts] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    if (!worker || !worker.deviceId) {
      setLoading(false);
      return;
    }

    async function loadDetails() {
      try {
        const [readingsData, alertsData] = await Promise.all([
          fetchReadings(worker.deviceId, 100),
          fetchAlerts()
        ]);
        setReadings(readingsData || []);
        // Filter alerts for this worker
        const workerAlerts = (alertsData || []).filter(a => a.deviceId === worker.deviceId);
        setAlerts(workerAlerts);
      } catch (err) {
        console.error('Failed to load worker detailed telemetry', err);
      } finally {
        setLoading(false);
      }
    }

    loadDetails();
  }, [worker]);

  if (!worker) return null;

  return (
    <div>
      <div style={{ marginBottom: '1.5rem', display: 'flex', alignItems: 'center', gap: '1rem' }}>
        <button className="nav-btn" onClick={onBack}>
          &larr; Back to Directory
        </button>
        <h2 style={{ fontSize: '1.4rem', fontWeight: 700 }}>
          {worker.name} ({worker.workerCode})
        </h2>
      </div>

      {/* Worker Profile Strip */}
      <div className="summary-card" style={{ marginBottom: '2rem' }}>
        <div style={{ display: 'flex', justifyContent: 'space-between', flexWrap: 'wrap', gap: '1rem' }}>
          <div>
            <span className="summary-card-title">Assigned Device</span>
            <div style={{ fontSize: '1.25rem', fontWeight: 700, fontFamily: 'var(--font-mono)' }}>
              {worker.deviceId || 'None'}
            </div>
          </div>
          <div>
            <span className="summary-card-title">Work Location</span>
            <div style={{ fontSize: '1.1rem', fontWeight: 600 }}>{worker.site}</div>
          </div>
          <div>
            <span className="summary-card-title">Assigned Role</span>
            <div style={{ fontSize: '1.1rem', fontWeight: 600 }}>{worker.role}</div>
          </div>
          <div>
            <span className="summary-card-title">Device Health</span>
            <div style={{ fontSize: '1.1rem', fontWeight: 600 }}>
              <span className={`status-badge ${worker.deviceStatus === 'ONLINE' ? 'normal' : 'offline'}`}>
                {worker.deviceStatus}
              </span>
              <span style={{ marginLeft: '0.75rem', fontFamily: 'var(--font-mono)' }}>
                {worker.battery != null ? `🔋 ${worker.battery}%` : '🔋 N/A'}
              </span>
            </div>
          </div>
        </div>
      </div>

      {/* Historical Time-Series Charts */}
      <h3 style={{ fontSize: '1.15rem', fontWeight: 700, marginBottom: '1rem' }}>
        Physiological Trend Telemetry
      </h3>

      <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(380px, 1fr))', gap: '1.25rem', marginBottom: '2.5rem' }}>
        {/* Heart Rate Chart */}
        <VitalsChart
          readings={readings}
          title="Heart Rate (Cardiovascular Stress)"
          metricKey="heartRate"
          unit="bpm"
          color="#38bdf8"
          threshold={110}
          thresholdLabel="Exertion Limit: 110 bpm"
        />

        {/* SpO2 Chart */}
        <VitalsChart
          readings={readings}
          title="Oxygen Saturation (SpO2 Respiratory)"
          metricKey="spo2"
          unit="%"
          color="#34d399"
          threshold={92}
          thresholdLabel="Hypoxia Limit: 92%"
        />

        {/* Temperature Chart */}
        <VitalsChart
          readings={readings}
          title="Skin / Body Temperature (Heat Stress)"
          metricKey="temperature"
          unit="°C"
          color="#f87171"
          threshold={38.0}
          thresholdLabel="Heat Stroke Limit: 38.0°C"
        />
      </div>

      {/* Incident Audit Trail */}
      <div className="section-header">
        <h3 style={{ fontSize: '1.15rem', fontWeight: 700 }}>
          Logged Incident Audit Trail ({alerts.length})
        </h3>
      </div>
      <AlertTable alerts={alerts} onAcknowledgeClick={onAcknowledgeClick} />
    </div>
  );
}
