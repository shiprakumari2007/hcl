# P_174 — Mac Test Baseline Report

**Date:** 2026-10-01  
**Machine:** MacBook (Apple Silicon / arm64)

---

## 1. macOS Environment

| Item | Value |
|:---|:---|
| **macOS Version** | 26.2 (Build 25C56) |
| **CPU Architecture** | Apple Silicon (arm64) |

---

## 2. Installed Software Versions

| Tool | Version | Install Method |
|:---|:---|:---|
| **Java JDK** | Oracle JDK 24.0.2 (via `java`), Homebrew OpenJDK 27 (via `mvn`) | System + Homebrew |
| **Apache Maven** | 3.9.16 | Homebrew |
| **Node.js** | v22.19.0 | Homebrew / nvm |
| **npm** | 10.9.3 | Bundled with Node.js |
| **PostgreSQL** | 18.6 | Homebrew |
| **Python** | 3.14.2 | Homebrew |
| **Git** | 2.50.1 (Apple Git-155) | Xcode CLT |
| **Docker** | 29.4.0 | Docker Desktop |
| **Docker Compose** | v5.1.2 | Docker Desktop |
| **Mosquitto (MQTT Broker)** | Installed via Homebrew (running) | Homebrew |

---

## 3. Pre-existing Project State

| Component | Status at Baseline |
|:---|:---|
| `backend/` | Complete Spring Boot 3.2.4 project with 31 Java sources, 2 test files |
| `frontend/` | Complete React 18 + Vite 5 SPA with 17 source files |
| `firmware/` | 3 Arduino sketches (Phase 2, Phase 3, MQTT) |
| `tools/` | Python + Node.js mock MQTT publishers, standalone test backend |
| `docs/` | 8 architecture/specification documents |
| `docker-compose.yml` | Complete 4-service stack definition |
| `.env.example` | Environment template with safe placeholder values |
| Database `safety_wearable_db` | **Did not exist** — needed creation |

---

## 4. Issues Found at Baseline

1. **PostgreSQL**: Database `safety_wearable_db` did not exist; needed creation + schema + seed.
2. **application.properties**: Default credentials were `postgres/postgres`; local Mac uses `shahabahmad` user with password auth.
3. **Maven tests**: Mockito/ByteBuddy failed on Java 27 (incompatible with default ByteBuddy version in Spring Boot 3.2.4). Required `-Dnet.bytebuddy.experimental=true`.
4. **paho-mqtt Python library**: Not installed; needed `pip3 install paho-mqtt`.
5. **paho-mqtt deprecation**: Python publisher uses MQTT Client API v1 (deprecated in paho-mqtt 2.x); functional but shows warning.
6. **Docker daemon**: Not running at baseline (Docker Desktop not started).
7. **Frontend node_modules**: Had pre-existing `node_modules` from a different platform (potentially Windows); reinstalled.
8. **No `docs/MAC_TEST_BASELINE.md`**: Did not exist.
9. **No `docs/MAC_VERIFICATION_REPORT.md`**: Did not exist.

---

## 5. Configuration Problems

| Problem | Resolution |
|:---|:---|
| DB user mismatch (`postgres` → `shahabahmad`) | Updated `application.properties` default fallback |
| DB password not configured | Passed via `SPRING_DATASOURCE_PASSWORD` env var at runtime |
| Java 27 + Mockito ByteBuddy | Added `-Dnet.bytebuddy.experimental=true` JVM flag |
| `paho-mqtt` missing | `pip3 install paho-mqtt` |

> [!NOTE]
> No passwords, API keys, or secrets are recorded in this document.
