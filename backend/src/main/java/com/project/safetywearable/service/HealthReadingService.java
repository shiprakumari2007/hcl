package com.project.safetywearable.service;

import com.project.safetywearable.dto.TelemetryPayloadDto;
import com.project.safetywearable.entity.HealthReading;
import com.project.safetywearable.repository.HealthReadingRepository;
import org.springframework.data.domain.PageRequest;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.List;

@Service
public class HealthReadingService {

    private final HealthReadingRepository healthReadingRepository;
    private final DeviceService deviceService;

    public HealthReadingService(HealthReadingRepository healthReadingRepository, DeviceService deviceService) {
        this.healthReadingRepository = healthReadingRepository;
        this.deviceService = deviceService;
    }

    @Transactional
    public HealthReading saveTelemetry(TelemetryPayloadDto dto) {
        // Record device presence
        deviceService.recordHeartbeat(dto.getDeviceId(), dto.getBattery());

        HealthReading reading = new HealthReading();
        reading.setDeviceId(dto.getDeviceId());
        reading.setTimestamp(dto.getTimestamp() != null ? dto.getTimestamp() : Instant.now());
        reading.setHeartRate(dto.getHeartRate());
        reading.setHeartRateValid(dto.getHeartRateValid());
        reading.setSpo2(dto.getSpo2());
        reading.setSpo2Valid(dto.getSpo2Valid());
        reading.setTemperature(dto.getTemperature());
        reading.setTemperatureValid(dto.getTemperatureValid());
        reading.setBattery(dto.getBattery());
        reading.setSignalQuality(dto.getSignalQuality());

        return healthReadingRepository.save(reading);
    }

    public List<HealthReading> getReadings(String deviceId, int limit) {
        int cappedLimit = Math.min(Math.max(limit, 1), 500);
        return healthReadingRepository.findByDeviceIdOrderByTimestampDesc(
            deviceId, PageRequest.of(0, cappedLimit)
        );
    }

    public List<HealthReading> getReadingsBetween(String deviceId, Instant from, Instant to) {
        return healthReadingRepository.findReadingsBetween(deviceId, from, to);
    }
}
