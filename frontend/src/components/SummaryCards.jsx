import React from 'react';

export default function SummaryCards({ summary }) {
  if (!summary) return null;

  return (
    <div className="summary-grid">
      <div className="summary-card">
        <span className="summary-card-title">Total Active Workers</span>
        <div className="summary-card-value">{summary.totalWorkers}</div>
        <span className="summary-card-sub">{summary.siteName || 'Civil Site Infrastructure'}</span>
      </div>

      <div className="summary-card">
        <span className="summary-card-title">Wearables Online</span>
        <div className="summary-card-value" style={{ color: 'var(--status-normal)' }}>
          {summary.onlineWorkers}
          <span style={{ fontSize: '1rem', color: 'var(--text-muted)' }}>
            / {summary.totalWorkers}
          </span>
        </div>
        <span className="summary-card-sub">
          {summary.offlineWorkers > 0 ? `${summary.offlineWorkers} devices offline` : 'All nodes transmitting'}
        </span>
      </div>

      <div className="summary-card">
        <span className="summary-card-title">Active Safety Alerts</span>
        <div className="summary-card-value" style={{ color: summary.activeAlerts > 0 ? 'var(--status-critical)' : 'var(--status-normal)' }}>
          {summary.activeAlerts}
        </div>
        <span className="summary-card-sub">
          {summary.activeAlerts > 0 ? 'Requires supervisor acknowledgment' : 'Zero unacknowledged hazards'}
        </span>
      </div>

      <div className="summary-card">
        <span className="summary-card-title">Elevated Warning Status</span>
        <div className="summary-card-value" style={{ color: summary.warningWorkers > 0 ? 'var(--status-warning)' : 'var(--text-main)' }}>
          {summary.warningWorkers}
        </div>
        <span className="summary-card-sub">
          Workers approaching fatigue/heat limits
        </span>
      </div>
    </div>
  );
}
