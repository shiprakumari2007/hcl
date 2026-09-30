import React, { useState } from 'react';

export default function AcknowledgeModal({ alert, onClose, onConfirm }) {
  const [supervisor, setSupervisor] = useState('Safety Officer Singh');
  const [note, setNote] = useState('');
  const [loading, setLoading] = useState(false);

  if (!alert) return null;

  const handleSubmit = async (e) => {
    e.preventDefault();
    setLoading(true);
    await onConfirm(alert.id, supervisor, note);
    setLoading(false);
  };

  return (
    <div className="modal-overlay" onClick={onClose}>
      <div className="modal-content" onClick={(e) => e.stopPropagation()}>
        <h3 style={{ fontSize: '1.15rem', marginBottom: '0.5rem', color: '#fff' }}>
          Acknowledge Safety Incident
        </h3>
        <p style={{ fontSize: '0.85rem', color: 'var(--text-muted)', marginBottom: '1.25rem' }}>
          Verify corrective action taken for <strong>{alert.alertType}</strong> on Device <strong>{alert.deviceId}</strong>.
        </p>

        <form onSubmit={handleSubmit}>
          <div className="form-group">
            <label className="form-label">Supervisor Name / ID</label>
            <input
              type="text"
              className="form-input"
              value={supervisor}
              onChange={(e) => setSupervisor(e.target.value)}
              required
            />
          </div>

          <div className="form-group">
            <label className="form-label">Remediation Note / Action Taken</label>
            <textarea
              className="form-input"
              rows={3}
              placeholder="e.g. Worker ordered to mandatory hydration station; vitals stabilized."
              value={note}
              onChange={(e) => setNote(e.target.value)}
            />
          </div>

          <div className="modal-actions">
            <button
              type="button"
              className="nav-btn"
              onClick={onClose}
              disabled={loading}
            >
              Cancel
            </button>
            <button
              type="submit"
              className="btn-primary"
              disabled={loading}
            >
              {loading ? 'Submitting...' : 'Confirm Acknowledgment'}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
