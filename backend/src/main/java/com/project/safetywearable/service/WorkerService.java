package com.project.safetywearable.service;

import com.project.safetywearable.dto.WorkerResponseDto;
import com.project.safetywearable.entity.Device;
import com.project.safetywearable.entity.HealthReading;
import com.project.safetywearable.entity.Worker;
import com.project.safetywearable.repository.DeviceRepository;
import com.project.safetywearable.repository.HealthReadingRepository;
import com.project.safetywearable.repository.WorkerRepository;
import org.springframework.stereotype.Service;

import java.util.List;
import java.util.Optional;
import java.util.stream.Collectors;

@Service
public class WorkerService {

    private final WorkerRepository workerRepository;
    private final DeviceRepository deviceRepository;
    private final HealthReadingRepository healthReadingRepository;

    public WorkerService(WorkerRepository workerRepository,
                         DeviceRepository deviceRepository,
                         HealthReadingRepository healthReadingRepository) {
        this.workerRepository = workerRepository;
        this.deviceRepository = deviceRepository;
        this.healthReadingRepository = healthReadingRepository;
    }

    public List<WorkerResponseDto> getAllWorkersWithVitals() {
        return workerRepository.findAll().stream()
            .map(this::enrichWorkerResponse)
            .collect(Collectors.toList());
    }

    public Optional<WorkerResponseDto> getWorkerById(Long id) {
        return workerRepository.findById(id).map(this::enrichWorkerResponse);
    }

    public Optional<Worker> getWorkerEntityByDeviceId(String deviceId) {
        return workerRepository.findByDeviceId(deviceId);
    }

    public long countAll() {
        return workerRepository.count();
    }

    public WorkerResponseDto enrichWorkerResponse(Worker worker) {
        WorkerResponseDto dto = new WorkerResponseDto();
        dto.setId(worker.getId());
        dto.setWorkerCode(worker.getWorkerCode());
        dto.setName(worker.getName());
        dto.setSite(worker.getSite());
        dto.setRole(worker.getRole());
        dto.setDeviceId(worker.getDeviceId());

        String deviceStatus = "OFFLINE";
        if (worker.getDeviceId() != null) {
            Optional<Device> deviceOpt = deviceRepository.findByDeviceId(worker.getDeviceId());
            if (deviceOpt.isPresent()) {
                deviceStatus = deviceOpt.get().getStatus();
                dto.setBattery(deviceOpt.get().getBattery());
            }

            Optional<HealthReading> readingOpt = healthReadingRepository
                .findFirstByDeviceIdOrderByTimestampDesc(worker.getDeviceId());

            if (readingOpt.isPresent()) {
                HealthReading r = readingOpt.get();
                dto.setCurrentHeartRate(r.getHeartRate());
                dto.setHeartRateValid(r.isHeartRateValid());
                dto.setCurrentSpo2(r.getSpo2());
                dto.setSpo2Valid(r.isSpo2Valid());
                dto.setCurrentTemperature(r.getTemperature());
                dto.setTemperatureValid(r.isTemperatureValid());
                dto.setSignalQuality(r.getSignalQuality());
                dto.setLastUpdated(r.getTimestamp());

                // Derive Safety Status
                dto.setSafetyStatus(deriveSafetyStatus(r));
            } else {
                dto.setSafetyStatus("NORMAL");
            }
        } else {
            dto.setSafetyStatus("UNASSIGNED");
        }

        dto.setDeviceStatus(deviceStatus);
        return dto;
    }

    private String deriveSafetyStatus(HealthReading r) {
        if (!r.isHeartRateValid() && !r.isSpo2Valid() && !r.isTemperatureValid()) {
            return "NORMAL";
        }
        boolean hrHigh = (r.isHeartRateValid() && r.getHeartRate() != null && r.getHeartRate() > 110);
        boolean spo2Low = (r.isSpo2Valid() && r.getSpo2() != null && r.getSpo2() < 92);
        boolean tempHigh = (r.isTemperatureValid() && r.getTemperature() != null && r.getTemperature() > 38.0);

        if (hrHigh || spo2Low || tempHigh) {
            return "WARNING";
        }
        return "NORMAL";
    }
}
