package com.project.safetywearable;

import com.project.safetywearable.dto.TelemetryPayloadDto;
import com.project.safetywearable.entity.Alert;
import com.project.safetywearable.repository.AlertRepository;
import com.project.safetywearable.repository.WorkerRepository;
import com.project.safetywearable.service.AlertEngineService;
import com.project.safetywearable.service.SseStreamingService;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.springframework.test.util.ReflectionTestUtils;

import static org.junit.jupiter.api.Assertions.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.*;

@ExtendWith(MockitoExtension.class)
public class AlertEngineServiceTest {

    @Mock
    private AlertRepository alertRepository;

    @Mock
    private WorkerRepository workerRepository;

    @Mock
    private SseStreamingService sseStreamingService;

    private AlertEngineService alertEngineService;

    @BeforeEach
    void setUp() {
        alertEngineService = new AlertEngineService(alertRepository, workerRepository, sseStreamingService);
        ReflectionTestUtils.setField(alertEngineService, "maxHeartRate", 110);
        ReflectionTestUtils.setField(alertEngineService, "minSpo2", 92);
        ReflectionTestUtils.setField(alertEngineService, "maxTemperature", 38.0);
        ReflectionTestUtils.setField(alertEngineService, "persistenceSeconds", 0); // 0s for immediate unit test triggering
        ReflectionTestUtils.setField(alertEngineService, "cooldownSeconds", 60);

        lenient().when(alertRepository.save(any(Alert.class))).thenAnswer(invocation -> {
            Alert a = invocation.getArgument(0);
            a.setId(101L);
            return a;
        });
    }

    @Test
    @DisplayName("Normal vitals within limits do NOT trigger safety alerts")
    void testNormalVitals_NoAlert() {
        TelemetryPayloadDto telemetry = new TelemetryPayloadDto();
        telemetry.setDeviceId("SW-001");
        telemetry.setHeartRate(82);
        telemetry.setHeartRateValid(true);
        telemetry.setSpo2(98);
        telemetry.setSpo2Valid(true);
        telemetry.setTemperature(37.0);
        telemetry.setTemperatureValid(true);

        alertEngineService.evaluateTelemetry(telemetry);

        verify(alertRepository, never()).save(any(Alert.class));
    }

    @Test
    @DisplayName("Elevated Heart Rate (>110 BPM) triggers HIGH_HEART_RATE alert")
    void testHighHeartRate_TriggersAlert() {
        TelemetryPayloadDto telemetry = new TelemetryPayloadDto();
        telemetry.setDeviceId("SW-001");
        telemetry.setHeartRate(124);
        telemetry.setHeartRateValid(true);
        telemetry.setSpo2(97);
        telemetry.setSpo2Valid(true);
        telemetry.setTemperature(37.2);
        telemetry.setTemperatureValid(true);

        alertEngineService.evaluateTelemetry(telemetry);

        ArgumentCaptor<Alert> captor = ArgumentCaptor.forClass(Alert.class);
        verify(alertRepository, times(1)).save(captor.capture());
        assertEquals("HIGH_HEART_RATE", captor.getValue().getAlertType());
        assertEquals(124.0, captor.getValue().getValue());
    }

    @Test
    @DisplayName("Low Oxygen Saturation (<92%) triggers CRITICAL LOW_SPO2 alert")
    void testLowSpo2_TriggersAlert() {
        TelemetryPayloadDto telemetry = new TelemetryPayloadDto();
        telemetry.setDeviceId("SW-002");
        telemetry.setHeartRate(88);
        telemetry.setHeartRateValid(true);
        telemetry.setSpo2(89);
        telemetry.setSpo2Valid(true);
        telemetry.setTemperature(37.1);
        telemetry.setTemperatureValid(true);

        alertEngineService.evaluateTelemetry(telemetry);

        ArgumentCaptor<Alert> captor = ArgumentCaptor.forClass(Alert.class);
        verify(alertRepository, times(1)).save(captor.capture());
        assertEquals("LOW_SPO2", captor.getValue().getAlertType());
        assertEquals("CRITICAL", captor.getValue().getSeverity());
    }

    @Test
    @DisplayName("Invalid / Disconnected sensor readings do NOT trigger false alerts")
    void testInvalidSensorReading_NoAlert() {
        TelemetryPayloadDto telemetry = new TelemetryPayloadDto();
        telemetry.setDeviceId("SW-001");
        telemetry.setHeartRate(null);
        telemetry.setHeartRateValid(false);
        telemetry.setSpo2(null);
        telemetry.setSpo2Valid(false);
        telemetry.setTemperature(36.8);
        telemetry.setTemperatureValid(true);
        telemetry.setSignalQuality("NO_CONTACT");

        alertEngineService.evaluateTelemetry(telemetry);

        verify(alertRepository, never()).save(any(Alert.class));
    }
}
