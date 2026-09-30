package com.project.safetywearable.mqtt;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.project.safetywearable.dto.AlertPayloadDto;
import com.project.safetywearable.dto.DeviceStatusPayloadDto;
import com.project.safetywearable.dto.TelemetryPayloadDto;
import com.project.safetywearable.service.AlertEngineService;
import com.project.safetywearable.service.DeviceService;
import com.project.safetywearable.service.HealthReadingService;
import com.project.safetywearable.service.SseStreamingService;
import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import org.eclipse.paho.client.mqttv3.*;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.nio.charset.StandardCharsets;

@Service
public class MqttSubscriberService implements MqttCallbackExtended {

    private static final Logger log = LoggerFactory.getLogger(MqttSubscriberService.class);

    @Value("${mqtt.broker.url:tcp://localhost:1883}")
    private String brokerUrl;

    @Value("${mqtt.client.id:SafetyWearableBackend}")
    private String clientId;

    @Value("${mqtt.username:}")
    private String username;

    @Value("${mqtt.password:}")
    private String password;

    @Value("${mqtt.topic.telemetry.filter:safetywearable/+/telemetry}")
    private String telemetryTopicFilter;

    @Value("${mqtt.topic.status.filter:safetywearable/+/status}")
    private String statusTopicFilter;

    @Value("${mqtt.topic.alert.filter:safetywearable/+/alert}")
    private String alertTopicFilter;

    private final HealthReadingService healthReadingService;
    private final AlertEngineService alertEngineService;
    private final DeviceService deviceService;
    private final SseStreamingService sseStreamingService;
    private final ObjectMapper objectMapper;

    private MqttAsyncClient mqttClient;

    public MqttSubscriberService(HealthReadingService healthReadingService,
                                 AlertEngineService alertEngineService,
                                 DeviceService deviceService,
                                 SseStreamingService sseStreamingService) {
        this.healthReadingService = healthReadingService;
        this.alertEngineService = alertEngineService;
        this.deviceService = deviceService;
        this.sseStreamingService = sseStreamingService;
        this.objectMapper = new ObjectMapper().registerModule(new JavaTimeModule());
    }

    @PostConstruct
    public void start() {
        try {
            mqttClient = new MqttAsyncClient(brokerUrl, clientId, new MemoryPersistence());
            mqttClient.setCallback(this);

            MqttConnectOptions options = new MqttConnectOptions();
            options.setCleanSession(true);
            options.setAutomaticReconnect(true);
            options.setConnectionTimeout(10);
            options.setKeepAliveInterval(30);

            if (username != null && !username.trim().isEmpty()) {
                options.setUserName(username);
            }
            if (password != null && !password.trim().isEmpty()) {
                options.setPassword(password.toCharArray());
            }

            log.info("[MQTT] Attempting connection to broker: {}", brokerUrl);
            mqttClient.connect(options, null, new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    log.info("[MQTT] Connected to broker successfully.");
                    subscribeToTopics();
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    log.warn("[MQTT] Initial connection to {} failed: {}. Will retry automatically.",
                        brokerUrl, exception.getMessage());
                }
            });

        } catch (MqttException e) {
            log.error("[MQTT] Error initializing async client: {}", e.getMessage());
        }
    }

    @Override
    public void connectComplete(boolean reconnect, String serverURI) {
        log.info("[MQTT] Connection established (reconnect={}) to {}", reconnect, serverURI);
        subscribeToTopics();
    }

    private void subscribeToTopics() {
        try {
            String[] topics = { telemetryTopicFilter, statusTopicFilter, alertTopicFilter };
            int[] qos = { 0, 1, 1 };
            mqttClient.subscribe(topics, qos);
            log.info("[MQTT] Subscribed to topic patterns: {}, {}, {}", topics[0], topics[1], topics[2]);
        } catch (MqttException e) {
            log.error("[MQTT] Failed to subscribe to topics: {}", e.getMessage());
        }
    }

    @Override
    public void connectionLost(Throwable cause) {
        log.warn("[MQTT] Connection lost: {}", cause != null ? cause.getMessage() : "Unknown");
    }

    @Override
    public void messageArrived(String topic, MqttMessage message) {
        String payload = new String(message.getPayload(), StandardCharsets.UTF_8);
        try {
            if (topic.endsWith("/telemetry")) {
                handleTelemetry(payload);
            } else if (topic.endsWith("/status")) {
                handleStatus(payload);
            } else if (topic.endsWith("/alert")) {
                handleAlert(payload);
            }
        } catch (Exception e) {
            log.error("[MQTT] Error processing payload on topic {}: {}", topic, e.getMessage());
        }
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken token) {
        // Not used for inbound subscriber
    }

    public void handleTelemetry(String json) {
        try {
            TelemetryPayloadDto dto = objectMapper.readValue(json, TelemetryPayloadDto.class);
            if (dto.getDeviceId() == null || dto.getDeviceId().trim().isEmpty()) {
                log.warn("[MQTT] Discarding telemetry missing deviceId");
                return;
            }

            // 1. Persist to PostgreSQL
            healthReadingService.saveTelemetry(dto);

            // 2. Evaluate Safety Alert Engine
            alertEngineService.evaluateTelemetry(dto);

            // 3. Broadcast to Connected Frontend Clients over SSE
            sseStreamingService.broadcastTelemetry(dto);

        } catch (Exception e) {
            log.error("[MQTT] Failed to deserialize telemetry: {}", e.getMessage());
        }
    }

    public void handleStatus(String json) {
        try {
            DeviceStatusPayloadDto dto = objectMapper.readValue(json, DeviceStatusPayloadDto.class);
            if (dto.getDeviceId() != null) {
                deviceService.registerOrUpdateDevice(
                    dto.getDeviceId(),
                    dto.getStatus() != null ? dto.getStatus() : "ONLINE",
                    dto.getFirmwareVersion(),
                    null
                );
                sseStreamingService.broadcastDeviceStatus(dto.getDeviceId(), dto.getStatus());
            }
        } catch (Exception e) {
            log.error("[MQTT] Failed to deserialize status payload: {}", e.getMessage());
        }
    }

    public void handleAlert(String json) {
        try {
            AlertPayloadDto dto = objectMapper.readValue(json, AlertPayloadDto.class);
            if (dto.getDeviceId() != null && dto.getAlertType() != null) {
                alertEngineService.recordAlert(
                    dto.getDeviceId(),
                    dto.getAlertType(),
                    dto.getSeverity() != null ? dto.getSeverity() : "WARNING",
                    dto.getValue(),
                    dto.getThreshold(),
                    dto.getMessage() != null ? dto.getMessage() : "Edge Safety Alert Dispatched"
                );
            }
        } catch (Exception e) {
            log.error("[MQTT] Failed to deserialize edge alert payload: {}", e.getMessage());
        }
    }

    @PreDestroy
    public void stop() {
        if (mqttClient != null && mqttClient.isConnected()) {
            try {
                mqttClient.disconnect();
                mqttClient.close();
            } catch (MqttException ignored) {}
        }
    }
}
