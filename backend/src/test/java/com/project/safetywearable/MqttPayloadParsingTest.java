package com.project.safetywearable;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.datatype.jsr310.JavaTimeModule;
import com.project.safetywearable.dto.TelemetryPayloadDto;
import org.junit.jupiter.api.DisplayName;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

public class MqttPayloadParsingTest {

    private final ObjectMapper mapper = new ObjectMapper().registerModule(new JavaTimeModule());

    @Test
    @DisplayName("Deserialize valid JSON telemetry with full biometric values")
    void testValidTelemetryDeserialization() throws Exception {
        String json = "{\"deviceId\":\"SW-001\",\"heartRate\":96,\"heartRateValid\":true,\"spo2\":97,\"spo2Valid\":true,\"temperature\":37.2,\"temperatureValid\":true,\"battery\":82,\"signalQuality\":\"VALID\"}";

        TelemetryPayloadDto dto = mapper.readValue(json, TelemetryPayloadDto.class);

        assertEquals("SW-001", dto.getDeviceId());
        assertEquals(96, dto.getHeartRate());
        assertTrue(dto.getHeartRateValid());
        assertEquals(97, dto.getSpo2());
        assertTrue(dto.getSpo2Valid());
        assertEquals(37.2, dto.getTemperature(), 0.001);
        assertTrue(dto.getTemperatureValid());
        assertEquals(82, dto.getBattery());
        assertEquals("VALID", dto.getSignalQuality());
    }

    @Test
    @DisplayName("Deserialize telemetry with null values on sensor disconnection")
    void testDisconnectedSensorDeserialization() throws Exception {
        String json = "{\"deviceId\":\"SW-002\",\"heartRate\":null,\"heartRateValid\":false,\"spo2\":null,\"spo2Valid\":false,\"temperature\":null,\"temperatureValid\":false,\"battery\":null,\"signalQuality\":\"NO_CONTACT\"}";

        TelemetryPayloadDto dto = mapper.readValue(json, TelemetryPayloadDto.class);

        assertEquals("SW-002", dto.getDeviceId());
        assertNull(dto.getHeartRate());
        assertFalse(dto.getHeartRateValid());
        assertNull(dto.getSpo2());
        assertFalse(dto.getSpo2Valid());
        assertNull(dto.getTemperature());
        assertFalse(dto.getTemperatureValid());
        assertEquals("NO_CONTACT", dto.getSignalQuality());
    }
}
