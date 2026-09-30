package com.project.safetywearable.service;

import com.project.safetywearable.entity.Device;
import com.project.safetywearable.repository.DeviceRepository;
import org.springframework.stereotype.Service;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.List;
import java.util.Optional;

@Service
public class DeviceService {

    private final DeviceRepository deviceRepository;

    public DeviceService(DeviceRepository deviceRepository) {
        this.deviceRepository = deviceRepository;
    }

    public List<Device> getAllDevices() {
        return deviceRepository.findAll();
    }

    public Optional<Device> getDeviceByDeviceId(String deviceId) {
        return deviceRepository.findByDeviceId(deviceId);
    }

    @Transactional
    public Device registerOrUpdateDevice(String deviceId, String status, String firmwareVersion, Integer battery) {
        Device device = deviceRepository.findByDeviceId(deviceId)
            .orElseGet(() -> new Device(deviceId, "Wearable Node " + deviceId, status));

        device.setStatus(status);
        device.setLastSeen(Instant.now());
        if (firmwareVersion != null) device.setFirmwareVersion(firmwareVersion);
        if (battery != null) device.setBattery(battery);

        return deviceRepository.save(device);
    }

    @Transactional
    public void recordHeartbeat(String deviceId, Integer battery) {
        deviceRepository.findByDeviceId(deviceId).ifPresent(device -> {
            device.setStatus("ONLINE");
            device.setLastSeen(Instant.now());
            if (battery != null) device.setBattery(battery);
            deviceRepository.save(device);
        });
    }

    public long countByStatus(String status) {
        return deviceRepository.countByStatus(status);
    }

    public long countAll() {
        return deviceRepository.count();
    }
}
