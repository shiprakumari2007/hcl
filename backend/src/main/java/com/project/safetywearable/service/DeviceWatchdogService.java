package com.project.safetywearable.service;

import com.project.safetywearable.entity.Device;
import com.project.safetywearable.repository.DeviceRepository;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Duration;
import java.time.Instant;
import java.util.List;

@Service
public class DeviceWatchdogService {

    private static final Logger log = LoggerFactory.getLogger(DeviceWatchdogService.class);

    private final DeviceRepository deviceRepository;
    private final SseStreamingService sseStreamingService;

    @Value("${safety.device.offline.timeout.seconds:30}")
    private int offlineTimeoutSeconds;

    public DeviceWatchdogService(DeviceRepository deviceRepository, SseStreamingService sseStreamingService) {
        this.deviceRepository = deviceRepository;
        this.sseStreamingService = sseStreamingService;
    }

    /**
     * Runs every 10 seconds to detect hardware nodes that stopped communicating.
     */
    @Scheduled(fixedRate = 10000)
    @Transactional
    public void scanDeviceHeartbeats() {
        Instant threshold = Instant.now().minus(Duration.ofSeconds(offlineTimeoutSeconds));
        List<Device> activeDevices = deviceRepository.findAll();

        for (Device device : activeDevices) {
            if ("ONLINE".equalsIgnoreCase(device.getStatus())) {
                if (device.getLastSeen() == null || device.getLastSeen().isBefore(threshold)) {
                    device.setStatus("OFFLINE");
                    deviceRepository.save(device);
                    log.info("[WATCHDOG] Device {} transitioned from ONLINE to OFFLINE (inactive > {}s)",
                        device.getDeviceId(), offlineTimeoutSeconds);

                    // Notify dashboard clients
                    sseStreamingService.broadcastDeviceStatus(device.getDeviceId(), "OFFLINE");
                }
            }
        }
    }
}
