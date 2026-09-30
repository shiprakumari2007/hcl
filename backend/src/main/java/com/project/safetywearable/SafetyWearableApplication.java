package com.project.safetywearable;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.scheduling.annotation.EnableScheduling;

@SpringBootApplication
@EnableScheduling
public class SafetyWearableApplication {

    public static void main(String[] args) {
        SpringApplication.run(SafetyWearableApplication.class, args);
    }
}
