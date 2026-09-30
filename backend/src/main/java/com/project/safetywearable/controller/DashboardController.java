package com.project.safetywearable.controller;

import com.project.safetywearable.dto.DashboardSummaryDto;
import com.project.safetywearable.service.AlertEngineService;
import com.project.safetywearable.service.DeviceService;
import com.project.safetywearable.service.WorkerService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/dashboard")
public class DashboardController {

    private final WorkerService workerService;
    private final DeviceService deviceService;
    private final AlertEngineService alertEngineService;

    public DashboardController(WorkerService workerService,
                               DeviceService deviceService,
                               AlertEngineService alertEngineService) {
        this.workerService = workerService;
        this.deviceService = deviceService;
        this.alertEngineService = alertEngineService;
    }

    @GetMapping("/summary")
    public ResponseEntity<DashboardSummaryDto> getSummary() {
        long totalWorkers = workerService.countAll();
        long onlineDevices = deviceService.countByStatus("ONLINE");
        long offlineDevices = deviceService.countByStatus("OFFLINE");
        long activeAlerts = alertEngineService.countActiveAlerts();

        long warningCount = workerService.getAllWorkersWithVitals().stream()
            .filter(w -> "WARNING".equalsIgnoreCase(w.getSafetyStatus()))
            .count();
        long normalCount = Math.max(0, totalWorkers - warningCount);

        DashboardSummaryDto summary = new DashboardSummaryDto(
            totalWorkers,
            onlineDevices,
            offlineDevices,
            activeAlerts,
            normalCount,
            warningCount,
            "Civil Construction Sector 62 Site"
        );

        return ResponseEntity.ok(summary);
    }
}
