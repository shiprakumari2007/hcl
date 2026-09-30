import React, { useState, useEffect } from 'react';
import Navbar from './components/Navbar';
import AlertBanner from './components/AlertBanner';
import AcknowledgeModal from './components/AcknowledgeModal';

import DashboardPage from './pages/DashboardPage';
import WorkersPage from './pages/WorkersPage';
import WorkerDetailPage from './pages/WorkerDetailPage';
import AlertsPage from './pages/AlertsPage';
import DevicesPage from './pages/DevicesPage';

import {
  fetchSummary,
  fetchWorkers,
  fetchAlerts,
  fetchDevices,
  acknowledgeAlert
} from './services/api';

import { subscribeToTelemetry } from './services/sse';

export default function App() {
  const [activeTab, setActiveTab] = useState('dashboard');
  const [selectedWorker, setSelectedWorker] = useState(null);

  const [summary, setSummary] = useState(null);
  const [workers, setWorkers] = useState([]);
  const [alerts, setAlerts] = useState([]);
  const [devices, setDevices] = useState([]);

  const [connected, setConnected] = useState(false);
  const [activeModalAlert, setActiveModalAlert] = useState(null);

  // Initial load & periodic REST refresh fallback
  async function refreshData() {
    try {
      const [sumData, workData, alertData, devData] = await Promise.all([
        fetchSummary(),
        fetchWorkers(),
        fetchAlerts(),
        fetchDevices()
      ]);
      setSummary(sumData);
      setWorkers(workData || []);
      setAlerts(alertData || []);
      setDevices(devData || []);
      setConnected(true);
    } catch (err) {
      console.error('[API POLL] Failed to fetch site data', err);
      // setConnected(false);
    }
  }

  useEffect(() => {
    refreshData();
    const interval = setInterval(refreshData, 3000); // 3-second heartbeat

    // Connect to real-time Server-Sent Events (SSE) stream
    const unsubscribe = subscribeToTelemetry(
      // On Telemetry Event
      (telemetry) => {
        setConnected(true);
        setWorkers((prevWorkers) =>
          prevWorkers.map((w) => {
            if (w.deviceId === telemetry.deviceId) {
              return {
                ...w,
                currentHeartRate: telemetry.heartRate,
                heartRateValid: telemetry.heartRateValid,
                currentSpo2: telemetry.spo2,
                spo2Valid: telemetry.spo2Valid,
                currentTemperature: telemetry.temperature,
                temperatureValid: telemetry.temperatureValid,
                battery: telemetry.battery != null ? telemetry.battery : w.battery,
                signalQuality: telemetry.signalQuality,
                deviceStatus: 'ONLINE',
                lastUpdated: new Date().toISOString()
              };
            }
            return w;
          })
        );
      },
      // On Alert Event
      (newAlert) => {
        setAlerts((prevAlerts) => [newAlert, ...prevAlerts]);
        setSummary((prev) => prev ? { ...prev, activeAlerts: prev.activeAlerts + 1 } : null);
      },
      // On Device Status Event
      (statusData) => {
        setDevices((prevDevices) =>
          prevDevices.map((d) =>
            d.deviceId === statusData.deviceId ? { ...d, status: statusData.status } : d
          )
        );
      }
    );

    return () => {
      clearInterval(interval);
      unsubscribe();
    };
  }, []);

  const handleSelectWorker = (worker) => {
    setSelectedWorker(worker);
    setActiveTab('worker-detail');
  };

  const handleConfirmAcknowledge = async (alertId, supervisor, note) => {
    try {
      await acknowledgeAlert(alertId, supervisor, note);
      setActiveModalAlert(null);
      await refreshData();
    } catch (err) {
      alert(`Error acknowledging incident: ${err.message}`);
    }
  };

  const unacknowledgedAlerts = alerts.filter((a) => !a.acknowledged);

  return (
    <div className="app-container">
      <Navbar
        activeTab={activeTab}
        setActiveTab={setActiveTab}
        activeAlertCount={unacknowledgedAlerts.length}
        connected={connected}
      />

      <AlertBanner
        activeAlerts={unacknowledgedAlerts}
        onViewAlerts={() => setActiveTab('alerts')}
      />

      <main className="main-content">
        {activeTab === 'dashboard' && (
          <DashboardPage
            summary={summary}
            workers={workers}
            onSelectWorker={handleSelectWorker}
            onViewAllWorkers={() => setActiveTab('workers')}
          />
        )}

        {activeTab === 'workers' && (
          <WorkersPage
            workers={workers}
            onSelectWorker={handleSelectWorker}
          />
        )}

        {activeTab === 'worker-detail' && (
          <WorkerDetailPage
            worker={selectedWorker}
            onBack={() => setActiveTab('workers')}
            onAcknowledgeClick={(a) => setActiveModalAlert(a)}
          />
        )}

        {activeTab === 'alerts' && (
          <AlertsPage
            alerts={alerts}
            onAcknowledgeClick={(a) => setActiveModalAlert(a)}
          />
        )}

        {activeTab === 'devices' && (
          <DevicesPage
            devices={devices}
            workers={workers}
          />
        )}
      </main>

      {/* Supervisor Acknowledgment Modal */}
      {activeModalAlert && (
        <AcknowledgeModal
          alert={activeModalAlert}
          onClose={() => setActiveModalAlert(null)}
          onConfirm={handleConfirmAcknowledge}
        />
      )}
    </div>
  );
}
