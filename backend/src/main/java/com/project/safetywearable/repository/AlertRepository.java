package com.project.safetywearable.repository;

import com.project.safetywearable.entity.Alert;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;

@Repository
public interface AlertRepository extends JpaRepository<Alert, Long> {
    List<Alert> findByAcknowledgedOrderByTimestampDesc(boolean acknowledged, Pageable pageable);
    List<Alert> findByDeviceIdOrderByTimestampDesc(String deviceId, Pageable pageable);
    List<Alert> findAllByOrderByTimestampDesc(Pageable pageable);
    long countByAcknowledged(boolean acknowledged);
}
