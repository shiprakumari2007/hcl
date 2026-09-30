package com.project.safetywearable.dto;

import java.time.Instant;

public class WorkerResponseDto {
    private Long id;
    private String workerCode;
    private String name;
    private String site;
    private String role;
    private String deviceId;
    private String deviceStatus;
    private Integer currentHeartRate;
    private Boolean heartRateValid;
    private Integer currentSpo2;
    private Boolean spo2Valid;
    private Double currentTemperature;
    private Boolean temperatureValid;
    private Integer battery;
    private String signalQuality;
    private String safetyStatus;
    private Instant lastUpdated;

    public WorkerResponseDto() {}

    public Long getId() { return id; }
    public void setId(Long id) { this.id = id; }

    public String getWorkerCode() { return workerCode; }
    public void setWorkerCode(String workerCode) { this.workerCode = workerCode; }

    public String getName() { return name; }
    public void setName(String name) { this.name = name; }

    public String getSite() { return site; }
    public void setSite(String site) { this.site = site; }

    public String getRole() { return role; }
    public void setRole(String role) { this.role = role; }

    public String getDeviceId() { return deviceId; }
    public void setDeviceId(String deviceId) { this.deviceId = deviceId; }

    public String getDeviceStatus() { return deviceStatus; }
    public void setDeviceStatus(String deviceStatus) { this.deviceStatus = deviceStatus; }

    public Integer getCurrentHeartRate() { return currentHeartRate; }
    public void setCurrentHeartRate(Integer currentHeartRate) { this.currentHeartRate = currentHeartRate; }

    public Boolean getHeartRateValid() { return heartRateValid; }
    public void setHeartRateValid(Boolean heartRateValid) { this.heartRateValid = heartRateValid; }

    public Integer getCurrentSpo2() { return currentSpo2; }
    public void setCurrentSpo2(Integer currentSpo2) { this.currentSpo2 = currentSpo2; }

    public Boolean getSpo2Valid() { return spo2Valid; }
    public void setSpo2Valid(Boolean spo2Valid) { this.spo2Valid = spo2Valid; }

    public Double getCurrentTemperature() { return currentTemperature; }
    public void setCurrentTemperature(Double currentTemperature) { this.currentTemperature = currentTemperature; }

    public Boolean getTemperatureValid() { return temperatureValid; }
    public void setTemperatureValid(Boolean temperatureValid) { this.temperatureValid = temperatureValid; }

    public Integer getBattery() { return battery; }
    public void setBattery(Integer battery) { this.battery = battery; }

    public String getSignalQuality() { return signalQuality; }
    public void setSignalQuality(String signalQuality) { this.signalQuality = signalQuality; }

    public String getSafetyStatus() { return safetyStatus; }
    public void setSafetyStatus(String safetyStatus) { this.safetyStatus = safetyStatus; }

    public Instant getLastUpdated() { return lastUpdated; }
    public void setLastUpdated(Instant lastUpdated) { this.lastUpdated = lastUpdated; }
}
