package com.project.safetywearable.controller;

import com.project.safetywearable.dto.AcknowledgeAlertRequestDto;
import com.project.safetywearable.dto.AlertPayloadDto;
import com.project.safetywearable.service.AlertEngineService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/alerts")
public class AlertController {

    private final AlertEngineService alertEngineService;

    public AlertController(AlertEngineService alertEngineService) {
        this.alertEngineService = alertEngineService;
    }

    @GetMapping
    public ResponseEntity<List<AlertPayloadDto>> getAlerts(
            @RequestParam(required = false) Boolean acknowledged,
            @RequestParam(required = false, defaultValue = "100") int limit) {
        return ResponseEntity.ok(alertEngineService.getAlerts(acknowledged, limit));
    }

    @GetMapping("/{deviceId}")
    public ResponseEntity<List<AlertPayloadDto>> getAlertsForDevice(
            @PathVariable String deviceId,
            @RequestParam(required = false, defaultValue = "50") int limit) {
        return ResponseEntity.ok(alertEngineService.getAlertsForDevice(deviceId, limit));
    }

    @PostMapping("/{id}/acknowledge")
    public ResponseEntity<AlertPayloadDto> acknowledgeAlert(
            @PathVariable Long id,
            @RequestBody(required = false) AcknowledgeAlertRequestDto request) {
        if (request == null) {
            request = new AcknowledgeAlertRequestDto();
            request.setSupervisor("Site Supervisor");
        }
        return alertEngineService.acknowledgeAlert(id, request)
            .map(ResponseEntity::ok)
            .orElse(ResponseEntity.notFound().build());
    }
}
