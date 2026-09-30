import React from 'react';

export default function Navbar({ activeTab, setActiveTab, activeAlertCount, connected }) {
  return (
    <header className="navbar">
      <div className="navbar-brand">
        <div className="brand-icon">P</div>
        <div>
          <span className="brand-title">P_174 Safety System</span>
          <span className="brand-subtitle">Occupational Health &amp; Hazard Monitor</span>
        </div>
      </div>

      <nav className="nav-links">
        <button
          className={`nav-btn ${activeTab === 'dashboard' ? 'active' : ''}`}
          onClick={() => setActiveTab('dashboard')}
        >
          Dashboard
        </button>
        <button
          className={`nav-btn ${activeTab === 'workers' ? 'active' : ''}`}
          onClick={() => setActiveTab('workers')}
        >
          Workers
        </button>
        <button
          className={`nav-btn ${activeTab === 'alerts' ? 'active' : ''}`}
          onClick={() => setActiveTab('alerts')}
        >
          Alerts
          {activeAlertCount > 0 && (
            <span style={{
              background: '#ef4444',
              color: '#fff',
              fontSize: '0.7rem',
              padding: '0.1rem 0.45rem',
              borderRadius: '9999px',
              marginLeft: '0.3rem',
              fontWeight: 700
            }}>
              {activeAlertCount}
            </span>
          )}
        </button>
        <button
          className={`nav-btn ${activeTab === 'devices' ? 'active' : ''}`}
          onClick={() => setActiveTab('devices')}
        >
          Devices
        </button>
      </nav>

      <div className="navbar-status">
        <div className="live-badge">
          <span className="pulse-dot"></span>
          <span>{connected ? 'LIVE TELEMETRY' : 'OFFLINE'}</span>
        </div>
      </div>
    </header>
  );
}
