#!/usr/bin/env python3
"""
============================================================================
P_174 Mock Telemetry & Scenario Generator
============================================================================
Simulates virtual worker wearables (e.g. SW-TEST-001, SW-TEST-002) for
comprehensive end-to-end testing of the MQTT broker, Spring Boot backend,
PostgreSQL database, and React frontend without physical hardware.

Usage:
    python mock_mqtt_publisher.py [--broker BROKER] [--port PORT] [--scenario SCENARIO]

Scenarios:
    normal        - All vitals stay within normal parameters (HR 70-85, SpO2 96-99, Temp 36.5-37.2)
    heat_stress   - Simulates rising body temperature leading to high temperature alert (>38.0°C)
    exhaustion    - Simulates rising heart rate leading to high HR alert (>110 BPM)
    hypoxia       - Simulates declining blood oxygen leading to low SpO2 alert (<92%)
    no_contact    - Simulates detached sensor (sends nulls and NO_CONTACT)
    all_random    - Continuous randomized drift across all parameters
============================================================================
"""

import sys
import time
import json
import random
import argparse
from datetime import datetime, timezone

try:
    import paho.mqtt.client as mqtt
except ImportError:
    print("[ERROR] paho-mqtt library not installed.")
    print("        Please run: pip install paho-mqtt")
    print("        Alternatively, use tools/mock_mqtt_publisher.js with Node.js.")
    sys.exit(1)

def generate_telemetry(device_id, scenario, step):
    now_iso = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    
    if scenario == "no_contact":
        return {
            "deviceId": device_id,
            "heartRate": None,
            "heartRateValid": False,
            "spo2": None,
            "spo2Valid": False,
            "temperature": None,
            "temperatureValid": False,
            "battery": 88,
            "signalQuality": "NO_CONTACT",
            "timestamp": now_iso
        }

    # Baseline healthy values
    hr = random.randint(72, 84)
    spo2 = random.randint(97, 99)
    temp = round(36.6 + random.uniform(0.0, 0.4), 1)

    if scenario == "heat_stress":
        # Gradually increase temperature past 38.0°C
        temp = round(37.5 + min(step * 0.2, 1.4), 1)
        hr = random.randint(95, 108)
    elif scenario == "exhaustion":
        # Gradually increase heart rate past 110 BPM
        hr = int(90 + min(step * 4, 34))
    elif scenario == "hypoxia":
        # Gradually drop SpO2 below 92%
        spo2 = int(96 - min(step * 1, 8))
    elif scenario == "all_random":
        hr = random.randint(65, 125)
        spo2 = random.randint(89, 99)
        temp = round(random.uniform(36.2, 38.8), 1)

    return {
        "deviceId": device_id,
        "heartRate": hr,
        "heartRateValid": True,
        "spo2": spo2,
        "spo2Valid": True,
        "temperature": temp,
        "temperatureValid": True,
        "battery": max(10, 100 - (step // 2)),
        "signalQuality": "VALID",
        "timestamp": now_iso
    }

def main():
    parser = argparse.ArgumentParser(description="P_174 Mock MQTT Telemetry Generator")
    parser.add_argument("--broker", default="localhost", help="MQTT Broker host (default: localhost)")
    parser.add_argument("--port", type=int, default=1883, help="MQTT Broker port (default: 1883)")
    parser.add_argument("--device", default="SW-TEST-001", help="Device ID to simulate")
    parser.add_argument("--scenario", default="normal",
                        choices=["normal", "heat_stress", "exhaustion", "hypoxia", "no_contact", "all_random"],
                        help="Telemetry test scenario")
    parser.add_argument("--interval", type=int, default=5, help="Publish interval in seconds")
    parser.add_argument("--count", type=int, default=0, help="Total messages to send (0 = infinite)")
    args = parser.parse_args()

    client = mqtt.Client(client_id=f"MockPublisher_{args.device}")
    
    print(f"\n==================================================")
    print(f" P_174 Mock Wearable Telemetry Generator")
    print(f" Target Broker : {args.broker}:{args.port}")
    print(f" Device ID     : {args.device}")
    print(f" Scenario      : {args.scenario}")
    print(f" Interval      : {args.interval}s")
    print(f"==================================================\n")

    try:
        client.connect(args.broker, args.port, 60)
        client.loop_start()
    except Exception as e:
        print(f"[FATAL] Could not connect to MQTT Broker {args.broker}:{args.port}: {e}")
        print("        Ensure your broker (Mosquitto/HiveMQ) is running.")
        sys.exit(1)

    # Publish Online Status
    topic_status = f"safetywearable/{args.device}/status"
    status_payload = {
        "deviceId": args.device,
        "status": "ONLINE",
        "firmwareVersion": "4.0.0-MOCK",
        "ipAddress": "127.0.0.1",
        "rssi": -45
    }
    client.publish(topic_status, json.dumps(status_payload), qos=1, retain=True)
    print(f"[STATUS] Published ONLINE state to {topic_status}")

    topic_telemetry = f"safetywearable/{args.device}/telemetry"
    step = 0

    try:
        while True:
            step += 1
            payload = generate_telemetry(args.device, args.scenario, step)
            client.publish(topic_telemetry, json.dumps(payload), qos=0)
            
            hr_str = f"{payload['heartRate']} bpm" if payload['heartRate'] is not None else "--"
            spo2_str = f"{payload['spo2']} %" if payload['spo2'] is not None else "--"
            temp_str = f"{payload['temperature']} C" if payload['temperature'] is not None else "--"
            
            print(f"[{step}] Dispatched -> HR: {hr_str:<7} | SpO2: {spo2_str:<6} | Temp: {temp_str:<7} | Quality: {payload['signalQuality']}")

            if args.count > 0 and step >= args.count:
                print("\n[DONE] Specified message count reached. Terminating simulation.")
                break

            time.sleep(args.interval)

    except KeyboardInterrupt:
        print("\n[STOPPING] Simulation interrupted by user.")
    finally:
        client.loop_stop()
        client.disconnect()
        print("[CLEANUP] MQTT connection cleanly closed.")

if __name__ == "__main__":
    main()
