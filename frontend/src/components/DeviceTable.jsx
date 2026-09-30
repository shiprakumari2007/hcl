import React from 'react';

export default function DeviceTable({ devices, workers }) {
  if (!devices || devices.length === 0) {
    return (
      <div className="table-container" style={{ padding: '2rem', textAlign: 'center', color: 'var(--text-muted)' }}>
        No wearable devices registered.
      </div>
    );
  }

  // Create lookup map from deviceId to assigned worker
  const workerMap = (workers || []).reduce((acc, w) => {
    if (w.deviceId) acc[w.deviceId] = w;
    return acc;
  }, {});

  return (
    <div className="table-container">
      <table>
        <thead>
          <tr>
            <th>Device ID</th>
            <th>Hardware Name</th>
            <th>Assigned Operator</th>
            <th>Connectivity</th>
            <th>Battery</th>
            <th>Firmware</th>
            <th>Last Transmitted</th>
          </tr>
        </thead>
        <tbody>
          {devices.map((device) => {
            const assigned = workerMap[device.deviceId];
            const isOnline = device.status === 'ONLINE';

            return (
              <tr key={device.id || device.deviceId}>
                <td>
                  <strong style={{ fontFamily: 'var(--font-mono)' }}>{device.deviceId}</strong>
                </td>
                <td>{device.name}</td>
                <td>
                  {assigned ? (
                    <div>
                      <strong>{assigned.name}</strong>
                      <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)', display: 'block' }}>
                        {assigned.workerCode} • {assigned.site}
                      </span>
                    </div>
                  ) : (
                    <span style={{ color: 'var(--text-dim)', fontStyle: 'italic' }}>Unassigned</span>
                  )}
                </td>
                <td>
                  <span className={`status-badge ${isOnline ? 'normal' : 'offline'}`}>
                    {device.status}
                  </span>
                </td>
                <td style={{ fontFamily: 'var(--font-mono)' }}>
                  {device.battery != null ? `🔋 ${device.battery}%` : '🔋 N/A'}
                </td>
                <td style={{ fontSize: '0.8rem', color: 'var(--text-muted)' }}>
                  {device.firmwareVersion || 'v4.0.0-MQTT'}
                </td>
                <td style={{ fontSize: '0.8rem' }}>
                  {device.lastSeen ? new Date(device.lastSeen).toLocaleTimeString() : 'Never'}
                </td>
              </tr>
            );
          })}
        </tbody>
      </table>
    </div>
  );
}
