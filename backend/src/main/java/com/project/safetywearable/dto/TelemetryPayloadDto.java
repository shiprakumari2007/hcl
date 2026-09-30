package com.project.safetywearable.dto;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import java.time.Instant;

@JsonIgnoreProperties(ignoreUnknown = true)
public class TelemetryPayloadDto {
    private String deviceId;
    private Integer heartRate;
    private Boolean heartRateValid;
    private Integer spo2;
    private Boolean spo2Valid;
    private Double temperature;
    private Boolean temperatureValid;
    private Integer battery;
    private String signalQuality;
    private Instant timestamp;

    public TelemetryPayloadDto() {}

    public String getDeviceId() { return deviceId; }
    public void setDeviceId(String deviceId) { this.deviceId = deviceId; }

    public Integer getHeartRate() { return heartRate; }
    public void setHeartRate(Integer heartRate) { this.heartRate = heartRate; }

    public Boolean getHeartRateValid() { return heartRateValid != null ? heartRateValid : false; }
    public void setHeartRateValid(Boolean heartRateValid) { this.heartRateValid = heartRateValid; }

    public Integer getSpo2() { return spo2; }
    public void setSpo2(Integer spo2) { this.spo2 = spo2; }

    public Boolean getSpo2Valid() { return spo2Valid != null ? spo2Valid : false; }
    public void setSpo2Valid(Boolean spo2Valid) { this.spo2Valid = spo2Valid; }

    public Double getTemperature() { return temperature; }
    public void setTemperature(Double temperature) { this.temperature = temperature; }

    public Boolean getTemperatureValid() { return temperatureValid != null ? temperatureValid : false; }
    public void setTemperatureValid(Boolean temperatureValid) { this.temperatureValid = temperatureValid; }

    public Integer getBattery() { return battery; }
    public void setBattery(Integer battery) { this.battery = battery; }

    public String getSignalQuality() { return signalQuality != null ? signalQuality : "VALID"; }
    public void setSignalQuality(String signalQuality) { this.signalQuality = signalQuality; }

    public Instant getTimestamp() { return timestamp != null ? timestamp : Instant.now(); }
    public void setTimestamp(Instant timestamp) { this.timestamp = timestamp; }
}
