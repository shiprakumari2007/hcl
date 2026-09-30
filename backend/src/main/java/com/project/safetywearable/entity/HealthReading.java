package com.project.safetywearable.entity;

import jakarta.persistence.*;
import java.time.Instant;

@Entity
@Table(name = "health_readings", indexes = {
    @Index(name = "idx_readings_device_time", columnList = "device_id, timestamp DESC"),
    @Index(name = "idx_readings_timestamp", columnList = "timestamp DESC")
})
public class HealthReading {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "device_id", nullable = false, length = 64)
    private String deviceId;

    @Column(name = "timestamp", nullable = false)
    private Instant timestamp = Instant.now();

    @Column(name = "heart_rate")
    private Integer heartRate;

    @Column(name = "heart_rate_valid", nullable = false)
    private boolean heartRateValid = false;

    @Column(name = "spo2")
    private Integer spo2;

    @Column(name = "spo2_valid", nullable = false)
    private boolean spo2Valid = false;

    @Column(name = "temperature")
    private Double temperature;

    @Column(name = "temperature_valid", nullable = false)
    private boolean temperatureValid = false;

    @Column(name = "battery")
    private Integer battery;

    @Column(name = "signal_quality", nullable = false, length = 32)
    private String signalQuality = "VALID";

    public HealthReading() {}

    public Long getId() { return id; }
    public void setId(Long id) { this.id = id; }

    public String getDeviceId() { return deviceId; }
    public void setDeviceId(String deviceId) { this.deviceId = deviceId; }

    public Instant getTimestamp() { return timestamp; }
    public void setTimestamp(Instant timestamp) { this.timestamp = timestamp; }

    public Integer getHeartRate() { return heartRate; }
    public void setHeartRate(Integer heartRate) { this.heartRate = heartRate; }

    public boolean isHeartRateValid() { return heartRateValid; }
    public void setHeartRateValid(boolean heartRateValid) { this.heartRateValid = heartRateValid; }

    public Integer getSpo2() { return spo2; }
    public void setSpo2(Integer spo2) { this.spo2 = spo2; }

    public boolean isSpo2Valid() { return spo2Valid; }
    public void setSpo2Valid(boolean spo2Valid) { this.spo2Valid = spo2Valid; }

    public Double getTemperature() { return temperature; }
    public void setTemperature(Double temperature) { this.temperature = temperature; }

    public boolean isTemperatureValid() { return temperatureValid; }
    public void setTemperatureValid(boolean temperatureValid) { this.temperatureValid = temperatureValid; }

    public Integer getBattery() { return battery; }
    public void setBattery(Integer battery) { this.battery = battery; }

    public String getSignalQuality() { return signalQuality; }
    public void setSignalQuality(String signalQuality) { this.signalQuality = signalQuality; }
}
