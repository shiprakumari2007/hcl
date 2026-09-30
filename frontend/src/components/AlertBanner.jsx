import React from 'react';

export default function AlertBanner({ activeAlerts, onViewAlerts }) {
  if (!activeAlerts || activeAlerts.length === 0) return null;

  const firstAlert = activeAlerts[0];

  return (
    <div className="emergency-banner">
      <div style={{ display: 'flex', alignItems: 'center', gap: '0.75rem' }}>
        <span style={{ fontSize: '1.25rem' }}>⚠️</span>
        <div>
          <span>
            <strong>ACTIVE HAZARD ALERT:</strong> {firstAlert.alertType} on Device {firstAlert.deviceId}
            {firstAlert.workerName ? ` (${firstAlert.workerName})` : ''} — {firstAlert.message}
          </span>
          {activeAlerts.length > 1 && (
            <span style={{ marginLeft: '0.5rem', opacity: 0.85 }}>
              (+{activeAlerts.length - 1} more active)
            </span>
          )}
        </div>
      </div>
      <button
        onClick={onViewAlerts}
        style={{
          background: '#fff',
          color: '#991b1b',
          border: 'none',
          padding: '0.35rem 0.75rem',
          borderRadius: '4px',
          fontWeight: 700,
          cursor: 'pointer',
          fontSize: '0.75rem'
        }}
      >
        View Incident Center
      </button>
    </div>
  );
}
