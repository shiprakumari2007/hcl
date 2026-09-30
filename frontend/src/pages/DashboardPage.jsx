import React from 'react';
import SummaryCards from '../components/SummaryCards';
import WorkerCard from '../components/WorkerCard';

export default function DashboardPage({ summary, workers, onSelectWorker, onViewAllWorkers }) {
  return (
    <div>
      <SummaryCards summary={summary} />

      <div className="section-header">
        <div>
          <h2 className="section-title">Site Labor Force &amp; Physiological Status</h2>
          <p style={{ fontSize: '0.85rem', color: 'var(--text-muted)' }}>
            Real-time biometric telemetry stream from active wearable nodes
          </p>
        </div>
        <button className="nav-btn" onClick={onViewAllWorkers}>
          View Detailed Roster &rarr;
        </button>
      </div>

      <div className="worker-grid">
        {(workers || []).map((worker) => (
          <WorkerCard
            key={worker.id}
            worker={worker}
            onSelect={onSelectWorker}
          />
        ))}
      </div>
    </div>
  );
}
