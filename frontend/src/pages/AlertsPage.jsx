import React, { useState } from 'react';
import AlertTable from '../components/AlertTable';

export default function AlertsPage({ alerts, onAcknowledgeClick }) {
  const [filter, setFilter] = useState('ALL');

  const filtered = (alerts || []).filter(a => {
    if (filter === 'ACTIVE') return !a.acknowledged;
    if (filter === 'ACKNOWLEDGED') return a.acknowledged;
    return true;
  });

  const activeCount = (alerts || []).filter(a => !a.acknowledged).length;

  return (
    <div>
      <div className="section-header">
        <div>
          <h2 className="section-title">Site Safety Alert Incident Center</h2>
          <p style={{ fontSize: '0.85rem', color: 'var(--text-muted)' }}>
            Real-time hazard notifications, threshold breaches, and supervisor acknowledgment logs
          </p>
        </div>

        <div style={{ display: 'flex', gap: '0.5rem' }}>
          <button
            className={`nav-btn ${filter === 'ALL' ? 'active' : ''}`}
            onClick={() => setFilter('ALL')}
          >
            All Incidents ({alerts.length})
          </button>
          <button
            className={`nav-btn ${filter === 'ACTIVE' ? 'active' : ''}`}
            onClick={() => setFilter('ACTIVE')}
          >
            Active ({activeCount})
          </button>
          <button
            className={`nav-btn ${filter === 'ACKNOWLEDGED' ? 'active' : ''}`}
            onClick={() => setFilter('ACKNOWLEDGED')}
          >
            Acknowledged
          </button>
        </div>
      </div>

      <AlertTable alerts={filtered} onAcknowledgeClick={onAcknowledgeClick} />
    </div>
  );
}
