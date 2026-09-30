package com.project.safetywearable.service;

import com.project.safetywearable.dto.AcknowledgeAlertRequestDto;
import com.project.safetywearable.dto.AlertPayloadDto;
import com.project.safetywearable.dto.TelemetryPayloadDto;
import com.project.safetywearable.entity.Alert;
import com.project.safetywearable.entity.Worker;
import com.project.safetywearable.repository.AlertRepository;
import com.project.safetywearable.repository.WorkerRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.data.domain.PageRequest;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

@Service
public class AlertEngineService {

    private static final Logger log = LoggerFactory.getLogger(AlertEngineService.class);

    private final AlertRepository alertRepository;
    private final WorkerRepository workerRepository;
    private final SseStreamingService sseStreamingService;

    @Value("${safety.threshold.heart-rate.max:110}")
    private int maxHeartRate;

    @Value("${safety.threshold.spo2.min:92}")
    private int minSpo2;

    @Value("${safety.threshold.temperature.max:38.0}")
    private double maxTemperature;

    @Value("${safety.alert.persistence.seconds:15}")
    private int persistenceSeconds;

    @Value("${safety.alert.cooldown.seconds:60}")
    private int cooldownSeconds;

    // Track when an abnormal condition started per device and alert type
    private final Map<String, Instant> abnormalStartMap = new ConcurrentHashMap<>();

    // Track when an alert was last recorded per device and alert type
    private final Map<String, Instant> lastAlertTimeMap = new ConcurrentHashMap<>();

    public AlertEngineService(AlertRepository alertRepository,
                              WorkerRepository workerRepository,
                              SseStreamingService sseStreamingService) {
        this.alertRepository = alertRepository;
        this.workerRepository = workerRepository;
        this.sseStreamingService = sseStreamingService;
    }

    /**
     * Evaluates incoming telemetry against configured safety limits.
     * Enforces the 15-second debounce persistence window and 60-second cooldown.
     */
    @Transactional
    public void evaluateTelemetry(TelemetryPayloadDto telemetry) {
        String deviceId = telemetry.getDeviceId();
        Instant now = Instant.now();

        // 1. High Heart Rate Check
        if (telemetry.getHeartRateValid() && telemetry.getHeartRate() != null && telemetry.getHeartRate() > maxHeartRate) {
            handleAbnormalCondition(deviceId, "HIGH_HEART_RATE", telemetry.getHeartRate().doubleValue(),
                (double) maxHeartRate, "WARNING",
                String.format("Persistent elevated Heart Rate (%d bpm) exceeding threshold (%d bpm)",
                    telemetry.getHeartRate(), maxHeartRate), now);
        } else {
            resetCondition(deviceId, "HIGH_HEART_RATE");
        }

        // 2. Low Oxygen Saturation (SpO2) Check
        if (telemetry.getSpo2Valid() && telemetry.getSpo2() != null && telemetry.getSpo2() < minSpo2) {
            handleAbnormalCondition(deviceId, "LOW_SPO2", telemetry.getSpo2().doubleValue(),
                (double) minSpo2, "CRITICAL",
                String.format("Arterial blood oxygen saturation dropped to %d%% (threshold: %d%%)",
                    telemetry.getSpo2(), minSpo2), now);
        } else {
            resetCondition(deviceId, "LOW_SPO2");
        }

        // 3. High Body Temperature / Heat Stress Check
        if (telemetry.getTemperatureValid() && telemetry.getTemperature() != null && telemetry.getTemperature() > maxTemperature) {
            handleAbnormalCondition(deviceId, "HIGH_TEMPERATURE", telemetry.getTemperature(),
                maxTemperature, "WARNING",
                String.format("Body temperature measured at %.1f°C indicating heat stress risk (threshold: %.1f°C)",
                    telemetry.getTemperature(), maxTemperature), now);
        } else {
            resetCondition(deviceId, "HIGH_TEMPERATURE");
        }
    }

    private void handleAbnormalCondition(String deviceId, String alertType, Double value, Double threshold,
                                        String severity, String message, Instant now) {
        String key = deviceId + ":" + alertType;

        // Start persistence timer if not already active
        abnormalStartMap.putIfAbsent(key, now);
        Instant firstDetected = abnormalStartMap.get(key);

        long secondsElapsed = java.time.Duration.between(firstDetected, now).getSeconds();

        if (secondsElapsed >= persistenceSeconds) {
            // Check cooldown window
            Instant lastAlert = lastAlertTimeMap.get(key);
            if (lastAlert == null || java.time.Duration.between(lastAlert, now).getSeconds() >= cooldownSeconds) {
                lastAlertTimeMap.put(key, now);
                recordAlert(deviceId, alertType, severity, value, threshold, message);
            }
        }
    }

    private void resetCondition(String deviceId, String alertType) {
        abnormalStartMap.remove(deviceId + ":" + alertType);
    }

    @Transactional
    public Alert recordAlert(String deviceId, String alertType, String severity, Double value, Double threshold, String message) {
        Alert alert = new Alert();
        alert.setDeviceId(deviceId);
        alert.setAlertType(alertType);
        alert.setSeverity(severity);
        alert.setValue(value);
        alert.setThreshold(threshold);
        alert.setMessage(message);
        alert.setTimestamp(Instant.now());
        alert.setAcknowledged(false);

        // Associate with worker if registered
        Optional<Worker> workerOpt = workerRepository.findByDeviceId(deviceId);
        workerOpt.ifPresent(worker -> alert.setWorkerId(worker.getId()));

        Alert saved = alertRepository.save(alert);
        log.warn("SAFETY ALERT CREATED [{}]: {} on Device {} (Value: {}, Threshold: {})",
            severity, alertType, deviceId, value, threshold);

        // Broadcast alert event over SSE to dashboard
        AlertPayloadDto dto = toDto(saved);
        sseStreamingService.broadcastAlert(dto);

        return saved;
    }

    @Transactional
    public Optional<AlertPayloadDto> acknowledgeAlert(Long id, AcknowledgeAlertRequestDto request) {
        return alertRepository.findById(id).map(alert -> {
            alert.setAcknowledged(true);
            alert.setAcknowledgedAt(Instant.now());
            alert.setAcknowledgedBy(request.getSupervisor() != null ? request.getSupervisor() : "Site Supervisor");
            Alert saved = alertRepository.save(alert);
            return toDto(saved);
        });
    }

    public List<AlertPayloadDto> getAlerts(Boolean acknowledgedOnly, int limit) {
        PageRequest page = PageRequest.of(0, Math.min(limit, 200));
        List<Alert> alerts;
        if (acknowledgedOnly != null) {
            alerts = alertRepository.findByAcknowledgedOrderByTimestampDesc(acknowledgedOnly, page);
        } else {
            alerts = alertRepository.findAllByOrderByTimestampDesc(page);
        }
        return alerts.stream().map(this::toDto).collect(Collectors.toList());
    }

    public List<AlertPayloadDto> getAlertsForDevice(String deviceId, int limit) {
        PageRequest page = PageRequest.of(0, Math.min(limit, 200));
        return alertRepository.findByDeviceIdOrderByTimestampDesc(deviceId, page)
            .stream().map(this::toDto).collect(Collectors.toList());
    }

    public long countActiveAlerts() {
        return alertRepository.countByAcknowledged(false);
    }

    private AlertPayloadDto toDto(Alert a) {
        AlertPayloadDto dto = new AlertPayloadDto();
        dto.setId(a.getId());
        dto.setDeviceId(a.getDeviceId());
        dto.setWorkerId(a.getWorkerId());
        dto.setAlertType(a.getAlertType());
        dto.setSeverity(a.getSeverity());
        dto.setValue(a.getValue());
        dto.setThreshold(a.getThreshold());
        dto.setMessage(a.getMessage());
        dto.setTimestamp(a.getTimestamp());
        dto.setAcknowledged(a.isAcknowledged());
        dto.setAcknowledgedAt(a.getAcknowledgedAt());
        dto.setAcknowledgedBy(a.getAcknowledgedBy());

        if (a.getWorkerId() != null) {
            workerRepository.findById(a.getWorkerId()).ifPresent(w -> {
                dto.setWorkerName(w.getName());
                dto.setWorkerCode(w.getWorkerCode());
            });
        }
        return dto;
    }
}
