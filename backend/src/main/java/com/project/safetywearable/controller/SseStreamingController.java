package com.project.safetywearable.controller;

import com.project.safetywearable.service.SseStreamingService;
import org.springframework.http.MediaType;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

@RestController
@RequestMapping("/api/stream")
public class SseStreamingController {

    private final SseStreamingService sseStreamingService;

    public SseStreamingController(SseStreamingService sseStreamingService) {
        this.sseStreamingService = sseStreamingService;
    }

    @GetMapping(value = "/telemetry", produces = MediaType.TEXT_EVENT_STREAM_VALUE)
    public SseEmitter streamTelemetry() {
        return sseStreamingService.registerClient();
    }
}
