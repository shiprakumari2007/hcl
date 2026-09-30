import React from 'react';
import DeviceTable from '../components/DeviceTable';

export default function DevicesPage({ devices, workers }) {
  const onlineCount = (devices || []).filter(d => d.status === 'ONLINE').length;

  return (
    <div>
      <div className="section-header">
        <div>
          <h2 className="section-title">Hardware Asset &amp; Wearable Fleet Manager</h2>
          <p style={{ fontSize: '0.85rem', color: 'var(--text-muted)' }}>
            Provisioned edge hardware nodes, firmware versions, battery life, and MQTT connectivity
          </p>
        </div>
        <div style={{ fontSize: '0.85rem', color: 'var(--text-muted)' }}>
          Online: <strong>{onlineCount}</strong> / {devices.length} Nodes
        </div>
      </div>

      <DeviceTable devices={devices} workers={workers} />
    </div>
  );
}
