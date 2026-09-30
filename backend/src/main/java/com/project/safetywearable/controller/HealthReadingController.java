package com.project.safetywearable.controller;

import com.project.safetywearable.entity.HealthReading;
import com.project.safetywearable.service.HealthReadingService;
import org.springframework.format.annotation.DateTimeFormat;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.time.Instant;
import java.util.List;

@RestController
@RequestMapping("/api/readings")
public class HealthReadingController {

    private final HealthReadingService healthReadingService;

    public HealthReadingController(HealthReadingService healthReadingService) {
        this.healthReadingService = healthReadingService;
    }

    @GetMapping("/{deviceId}")
    public ResponseEntity<List<HealthReading>> getReadings(
            @PathVariable String deviceId,
            @RequestParam(required = false, defaultValue = "100") int limit,
            @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) Instant from,
            @RequestParam(required = false) @DateTimeFormat(iso = DateTimeFormat.ISO.DATE_TIME) Instant to) {

        if (from != null && to != null) {
            return ResponseEntity.ok(healthReadingService.getReadingsBetween(deviceId, from, to));
        }

        return ResponseEntity.ok(healthReadingService.getReadings(deviceId, limit));
    }
}
