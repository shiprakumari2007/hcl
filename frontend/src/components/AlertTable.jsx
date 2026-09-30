import React from 'react';

export default function AlertTable({ alerts, onAcknowledgeClick }) {
  if (!alerts || alerts.length === 0) {
    return (
      <div className="table-container" style={{ padding: '2rem', textAlign: 'center', color: 'var(--text-muted)' }}>
        No safety alerts logged for this period.
      </div>
    );
  }

  return (
    <div className="table-container">
      <table>
        <thead>
          <tr>
            <th>Timestamp</th>
            <th>Worker / Device</th>
            <th>Hazard Type</th>
            <th>Severity</th>
            <th>Value vs Limit</th>
            <th>Message</th>
            <th>Status / Action</th>
          </tr>
        </thead>
        <tbody>
          {alerts.map((alert) => {
            const timeStr = new Date(alert.timestamp).toLocaleTimeString();
            const dateStr = new Date(alert.timestamp).toLocaleDateString();

            return (
              <tr key={alert.id}>
                <td>
                  <div style={{ fontWeight: 600 }}>{timeStr}</div>
                  <div style={{ fontSize: '0.7rem', color: 'var(--text-dim)' }}>{dateStr}</div>
                </td>
                <td>
                  <strong>{alert.workerName || 'Unassigned Worker'}</strong>
                  <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>
                    Device: {alert.deviceId}
                  </div>
                </td>
                <td>
                  <span style={{ fontWeight: 600, color: alert.severity === 'CRITICAL' ? 'var(--status-critical)' : 'var(--status-warning)' }}>
                    {alert.alertType}
                  </span>
                </td>
                <td>
                  <span className={`status-badge ${alert.severity === 'CRITICAL' ? 'critical' : 'warning'}`}>
                    {alert.severity}
                  </span>
                </td>
                <td style={{ fontFamily: 'var(--font-mono)' }}>
                  {alert.value != null ? alert.value : '--'} / {alert.threshold != null ? alert.threshold : '--'}
                </td>
                <td style={{ maxWidth: '300px', fontSize: '0.8rem' }}>
                  {alert.message}
                </td>
                <td>
                  {alert.acknowledged ? (
                    <div>
                      <span className="status-badge normal">ACKNOWLEDGED</span>
                      <div style={{ fontSize: '0.7rem', color: 'var(--text-dim)', marginTop: '2px' }}>
                        By {alert.acknowledgedBy || 'Supervisor'}
                      </div>
                    </div>
                  ) : (
                    <button
                      className="btn-primary"
                      onClick={() => onAcknowledgeClick(alert)}
                    >
                      Acknowledge
                    </button>
                  )}
                </td>
              </tr>
            );
          })}
        </tbody>
      </table>
    </div>
  );
}
