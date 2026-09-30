package com.project.safetywearable.repository;

import com.project.safetywearable.entity.Worker;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

@Repository
public interface WorkerRepository extends JpaRepository<Worker, Long> {
    Optional<Worker> findByWorkerCode(String workerCode);
    Optional<Worker> findByDeviceId(String deviceId);
}
