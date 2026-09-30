/**
 * ============================================================================
 * P_174 Frontend REST API Client
 * ============================================================================
 */

const BASE_URL = import.meta.env.VITE_API_BASE_URL || '/api';

export async function fetchSummary() {
  const res = await fetch(`${BASE_URL}/dashboard/summary`);
  if (!res.ok) throw new Error(`Failed to fetch summary: ${res.statusText}`);
  return res.json();
}

export async function fetchWorkers() {
  const res = await fetch(`${BASE_URL}/workers`);
  if (!res.ok) throw new Error(`Failed to fetch workers: ${res.statusText}`);
  return res.json();
}

export async function fetchWorkerById(id) {
  const res = await fetch(`${BASE_URL}/workers/${id}`);
  if (!res.ok) throw new Error(`Failed to fetch worker #${id}: ${res.statusText}`);
  return res.json();
}

export async function fetchDevices() {
  const res = await fetch(`${BASE_URL}/devices`);
  if (!res.ok) throw new Error(`Failed to fetch devices: ${res.statusText}`);
  return res.json();
}

export async function fetchAlerts(acknowledged = null) {
  const url = acknowledged !== null 
    ? `${BASE_URL}/alerts?acknowledged=${acknowledged}`
    : `${BASE_URL}/alerts`;
  const res = await fetch(url);
  if (!res.ok) throw new Error(`Failed to fetch alerts: ${res.statusText}`);
  return res.json();
}

export async function fetchReadings(deviceId, limit = 100) {
  const res = await fetch(`${BASE_URL}/readings/${deviceId}?limit=${limit}`);
  if (!res.ok) throw new Error(`Failed to fetch readings for ${deviceId}: ${res.statusText}`);
  return res.json();
}

export async function acknowledgeAlert(alertId, supervisor, note) {
  const res = await fetch(`${BASE_URL}/alerts/${alertId}/acknowledge`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ supervisor, note })
  });
  if (!res.ok) throw new Error(`Failed to acknowledge alert: ${res.statusText}`);
  return res.json();
}
