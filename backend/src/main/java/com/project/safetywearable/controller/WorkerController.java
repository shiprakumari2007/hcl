package com.project.safetywearable.controller;

import com.project.safetywearable.dto.WorkerResponseDto;
import com.project.safetywearable.service.WorkerService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.List;

@RestController
@RequestMapping("/api/workers")
public class WorkerController {

    private final WorkerService workerService;

    public WorkerController(WorkerService workerService) {
        this.workerService = workerService;
    }

    @GetMapping
    public ResponseEntity<List<WorkerResponseDto>> getAllWorkers() {
        return ResponseEntity.ok(workerService.getAllWorkersWithVitals());
    }

    @GetMapping("/{id}")
    public ResponseEntity<WorkerResponseDto> getWorkerById(@PathVariable Long id) {
        return workerService.getWorkerById(id)
            .map(ResponseEntity::ok)
            .orElse(ResponseEntity.notFound().build());
    }
}
