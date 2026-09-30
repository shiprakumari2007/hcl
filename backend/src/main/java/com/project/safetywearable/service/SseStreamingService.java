package com.project.safetywearable.service;

import com.project.safetywearable.dto.AlertPayloadDto;
import com.project.safetywearable.dto.TelemetryPayloadDto;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Service;
import org.springframework.web.servlet.mvc.method.annotation.SseEmitter;

import java.io.IOException;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CopyOnWriteArrayList;

@Service
public class SseStreamingService {

    private static final Logger log = LoggerFactory.getLogger(SseStreamingService.class);

    private final List<SseEmitter> emitters = new CopyOnWriteArrayList<>();

    public SseEmitter registerClient() {
        // 30 minute timeout
        SseEmitter emitter = new SseEmitter(1800000L);

        emitters.add(emitter);
        emitter.onCompletion(() -> emitters.remove(emitter));
        emitter.onTimeout(() -> emitters.remove(emitter));
        emitter.onError(e -> emitters.remove(emitter));

        try {
            emitter.send(SseEmitter.event().name("connected").data("Real-Time Telemetry Stream Connected"));
        } catch (IOException e) {
            emitters.remove(emitter);
        }

        return emitter;
    }

    public void broadcastTelemetry(TelemetryPayloadDto telemetry) {
        broadcast("telemetry", telemetry);
    }

    public void broadcastAlert(AlertPayloadDto alert) {
        broadcast("alert", alert);
    }

    public void broadcastDeviceStatus(String deviceId, String status) {
        broadcast("status", Map.of("deviceId", deviceId, "status", status));
    }

    private void broadcast(String eventName, Object data) {
        for (SseEmitter emitter : emitters) {
            try {
                emitter.send(SseEmitter.event().name(eventName).data(data));
            } catch (Exception e) {
                emitters.remove(emitter);
            }
        }
    }
}
