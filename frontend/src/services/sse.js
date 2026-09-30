/**
 * ============================================================================
 * P_174 Frontend Server-Sent Events (SSE) Client
 * ============================================================================
 */

export function subscribeToTelemetry(onTelemetry, onAlert, onStatus) {
  const url = '/api/stream/telemetry';
  const eventSource = new EventSource(url);

  eventSource.addEventListener('telemetry', (e) => {
    try {
      const data = JSON.parse(e.data);
      if (onTelemetry) onTelemetry(data);
    } catch (err) {
      console.error('[SSE] Failed to parse telemetry event', err);
    }
  });

  eventSource.addEventListener('alert', (e) => {
    try {
      const data = JSON.parse(e.data);
      if (onAlert) onAlert(data);
    } catch (err) {
      console.error('[SSE] Failed to parse alert event', err);
    }
  });

  eventSource.addEventListener('status', (e) => {
    try {
      const data = JSON.parse(e.data);
      if (onStatus) onStatus(data);
    } catch (err) {
      console.error('[SSE] Failed to parse status event', err);
    }
  });

  eventSource.onerror = (err) => {
    // SSE will automatically attempt reconnection
    // console.warn('[SSE] EventSource disconnected, retrying...', err);
  };

  return () => {
    eventSource.close();
  };
}
