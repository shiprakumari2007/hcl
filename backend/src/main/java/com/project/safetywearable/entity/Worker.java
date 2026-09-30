package com.project.safetywearable.entity;

import jakarta.persistence.*;
import java.time.Instant;

@Entity
@Table(name = "workers")
public class Worker {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "worker_code", nullable = false, unique = true, length = 64)
    private String workerCode;

    @Column(name = "name", nullable = false, length = 128)
    private String name;

    @Column(name = "site", nullable = false, length = 128)
    private String site;

    @Column(name = "role", length = 64)
    private String role = "Construction Worker";

    @Column(name = "device_id", length = 64)
    private String deviceId;

    @Column(name = "created_at", nullable = false, updatable = false)
    private Instant createdAt = Instant.now();

    public Worker() {}

    public Worker(String workerCode, String name, String site, String role, String deviceId) {
        this.workerCode = workerCode;
        this.name = name;
        this.site = site;
        this.role = role;
        this.deviceId = deviceId;
        this.createdAt = Instant.now();
    }

    public Long getId() { return id; }
    public void setId(Long id) { this.id = id; }

    public String getWorkerCode() { return workerCode; }
    public void setWorkerCode(String workerCode) { this.workerCode = workerCode; }

    public String getName() { return name; }
    public void setName(String name) { this.name = name; }

    public String getSite() { return site; }
    public void setSite(String site) { this.site = site; }

    public String getRole() { return role; }
    public void setRole(String role) { this.role = role; }

    public String getDeviceId() { return deviceId; }
    public void setDeviceId(String deviceId) { this.deviceId = deviceId; }

    public Instant getCreatedAt() { return createdAt; }
    public void setCreatedAt(Instant createdAt) { this.createdAt = createdAt; }
}
