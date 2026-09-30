import React from 'react';

export default function WorkerCard({ worker, onSelect }) {
  const isOnline = worker.deviceStatus === 'ONLINE';
  const hasHR = worker.heartRateValid && worker.currentHeartRate != null;
  const hasSpo2 = worker.spo2Valid && worker.currentSpo2 != null;
  const hasTemp = worker.temperatureValid && worker.currentTemperature != null;

  // Determine badge styling
  let badgeClass = 'offline';
  let badgeText = 'OFFLINE';

  if (isOnline) {
    if (worker.safetyStatus === 'WARNING') {
      badgeClass = 'warning';
      badgeText = 'WARNING';
    } else {
      badgeClass = 'normal';
      badgeText = 'NORMAL';
    }
  }

  return (
    <div
      className={`worker-card status-${badgeClass}`}
      onClick={() => onSelect(worker)}
    >
      <div className="card-top">
        <div>
          <h3 className="worker-name">{worker.name}</h3>
          <span className="worker-meta">
            {worker.workerCode} • {worker.role}
          </span>
          <div style={{ fontSize: '0.75rem', color: 'var(--text-dim)', marginTop: '0.2rem' }}>
            {worker.site}
          </div>
        </div>
        <span className={`status-badge ${badgeClass}`}>{badgeText}</span>
      </div>

      <div className="vitals-grid">
        <div className="vital-item">
          <span className="vital-label">HEART RATE</span>
          <span className={`vital-value ${!hasHR ? 'invalid' : ''}`}>
            {hasHR ? `${worker.currentHeartRate}` : '--'}
            {hasHR && <span style={{ fontSize: '0.7rem', color: 'var(--text-muted)', marginLeft: '2px' }}>bpm</span>}
          </span>
        </div>

        <div className="vital-item">
          <span className="vital-label">SPO2</span>
          <span className={`vital-value ${!hasSpo2 ? 'invalid' : ''}`}>
            {hasSpo2 ? `${worker.currentSpo2}` : '--'}
            {hasSpo2 && <span style={{ fontSize: '0.7rem', color: 'var(--text-muted)', marginLeft: '2px' }}>%</span>}
          </span>
        </div>

        <div className="vital-item">
          <span className="vital-label">BODY TEMP</span>
          <span className={`vital-value ${!hasTemp ? 'invalid' : ''}`}>
            {hasTemp ? `${worker.currentTemperature.toFixed(1)}` : '--'}
            {hasTemp && <span style={{ fontSize: '0.7rem', color: 'var(--text-muted)', marginLeft: '2px' }}>°C</span>}
          </span>
        </div>
      </div>

      <div className="card-footer">
        <span>Device: <strong>{worker.deviceId || 'Unassigned'}</strong></span>
        <span>
          {worker.battery != null ? `🔋 ${worker.battery}%` : '🔋 N/A'}
        </span>
        <span>
          Quality: <strong style={{ color: worker.signalQuality === 'VALID' ? 'var(--status-normal)' : 'var(--text-dim)' }}>
            {worker.signalQuality || 'N/A'}
          </strong>
        </span>
      </div>
    </div>
  );
}
