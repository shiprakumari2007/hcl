package com.project.safetywearable.repository;

import com.project.safetywearable.entity.HealthReading;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import java.time.Instant;
import java.util.List;
import java.util.Optional;

@Repository
public interface HealthReadingRepository extends JpaRepository<HealthReading, Long> {

    List<HealthReading> findByDeviceIdOrderByTimestampDesc(String deviceId, Pageable pageable);

    @Query("SELECT r FROM HealthReading r WHERE r.deviceId = :deviceId AND r.timestamp BETWEEN :from AND :to ORDER BY r.timestamp ASC")
    List<HealthReading> findReadingsBetween(
        @Param("deviceId") String deviceId,
        @Param("from") Instant from,
        @Param("to") Instant to
    );

    Optional<HealthReading> findFirstByDeviceIdOrderByTimestampDesc(String deviceId);
}
