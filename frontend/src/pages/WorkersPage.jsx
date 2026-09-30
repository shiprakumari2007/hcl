import React, { useState } from 'react';
import WorkerCard from '../components/WorkerCard';

export default function WorkersPage({ workers, onSelectWorker }) {
  const [filterSite, setFilterSite] = useState('ALL');
  const [filterStatus, setFilterStatus] = useState('ALL');

  const sites = ['ALL', ...new Set((workers || []).map(w => w.site).filter(Boolean))];

  const filtered = (workers || []).filter(w => {
    if (filterSite !== 'ALL' && w.site !== filterSite) return false;
    if (filterStatus !== 'ALL') {
      if (filterStatus === 'ONLINE' && w.deviceStatus !== 'ONLINE') return false;
      if (filterStatus === 'OFFLINE' && w.deviceStatus === 'ONLINE') return false;
      if (filterStatus === 'WARNING' && w.safetyStatus !== 'WARNING') return false;
    }
    return true;
  });

  return (
    <div>
      <div className="section-header">
        <div>
          <h2 className="section-title">Construction Workforce Directory</h2>
          <p style={{ fontSize: '0.85rem', color: 'var(--text-muted)' }}>
            Biometric telemetry, assigned wearables, and active hazard state
          </p>
        </div>

        {/* Filters */}
        <div style={{ display: 'flex', gap: '0.75rem' }}>
          <select
            className="form-input"
            style={{ width: 'auto', padding: '0.35rem 0.6rem', fontSize: '0.8rem' }}
            value={filterSite}
            onChange={(e) => setFilterSite(e.target.value)}
          >
            {sites.map(s => <option key={s} value={s}>Site: {s}</option>)}
          </select>

          <select
            className="form-input"
            style={{ width: 'auto', padding: '0.35rem 0.6rem', fontSize: '0.8rem' }}
            value={filterStatus}
            onChange={(e) => setFilterStatus(e.target.value)}
          >
            <option value="ALL">Status: All</option>
            <option value="ONLINE">Online Only</option>
            <option value="OFFLINE">Offline Only</option>
            <option value="WARNING">Warnings Only</option>
          </select>
        </div>
      </div>

      <div className="worker-grid">
        {filtered.map(w => (
          <WorkerCard key={w.id} worker={w} onSelect={onSelectWorker} />
        ))}
      </div>

      {filtered.length === 0 && (
        <div className="table-container" style={{ padding: '3rem', textAlign: 'center', color: 'var(--text-muted)' }}>
          No workers match the selected criteria.
        </div>
      )}
    </div>
  );
}
