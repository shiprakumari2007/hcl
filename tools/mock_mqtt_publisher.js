#!/usr/bin/env node
/**
 * ============================================================================
 * P_174 Mock Telemetry & Scenario Generator (Node.js)
 * ============================================================================
 * Runs on Node.js using standard TCP sockets or standard mqtt npm package.
 * Requires: npm install mqtt (or run with mock test runner)
 * ============================================================================
 */

const mqtt = require('mqtt');

const brokerUrl = process.env.MQTT_BROKER_URL || 'mqtt://localhost:1883';
const deviceId = process.argv[2] || 'SW-TEST-001';
const scenario = process.argv[3] || 'normal';

console.log(`\n==================================================`);
console.log(` P_174 Node.js Mock Wearable Telemetry Generator`);
console.log(` Broker URL : ${brokerUrl}`);
console.log(` Device ID  : ${deviceId}`);
console.log(` Scenario   : ${scenario}`);
console.log(`==================================================\n`);

const client = mqtt.connect(brokerUrl, {
    clientId: `NodeMock_${deviceId}`,
    clean: true,
    connectTimeout: 5000
});

client.on('connect', () => {
    console.log(`[CONNECTED] Connected to MQTT broker.`);

    // 1. Publish Status
    const statusTopic = `safetywearable/${deviceId}/status`;
    const statusPayload = JSON.stringify({
        deviceId: deviceId,
        status: "ONLINE",
        firmwareVersion: "4.0.0-NODE-MOCK",
        ipAddress: "127.0.0.1",
        rssi: -50
    });
    client.publish(statusTopic, statusPayload, { qos: 1, retain: true });

    // 2. Loop Telemetry
    let step = 0;
    const telemetryTopic = `safetywearable/${deviceId}/telemetry`;

    setInterval(() => {
        step++;
        let hr = Math.floor(70 + Math.random() * 15);
        let spo2 = Math.floor(96 + Math.random() * 3);
        let temp = +(36.6 + Math.random() * 0.5).toFixed(1);

        if (scenario === 'heat_stress') {
            temp = +(37.6 + Math.min(step * 0.2, 1.3)).toFixed(1);
        } else if (scenario === 'exhaustion') {
            hr = Math.floor(92 + Math.min(step * 4, 32));
        } else if (scenario === 'hypoxia') {
            spo2 = Math.floor(96 - Math.min(step * 1, 8));
        }

        const telemetry = {
            deviceId: deviceId,
            heartRate: hr,
            heartRateValid: true,
            spo2: spo2,
            spo2Valid: true,
            temperature: temp,
            temperatureValid: true,
            battery: 85,
            signalQuality: "VALID",
            timestamp: new Date().toISOString()
        };

        const json = JSON.stringify(telemetry);
        client.publish(telemetryTopic, json);
        console.log(`[${step}] Dispatched -> HR: ${hr} bpm | SpO2: ${spo2}% | Temp: ${temp}°C`);
    }, 5000);
});

client.on('error', (err) => {
    console.error(`[MQTT ERROR] ${err.message}`);
});
