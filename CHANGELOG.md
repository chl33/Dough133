# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.1.0] - 2026-11-01

### Added
- **Software-Monitored Hardware Safety Watchdog**: Replaced the autonomous hardware LEDC PWM safety signal on `GPIO19` with a `200 Hz` `esp_timer` pulse generator gated by both `loop()` (`250 ms` timeout) and `TempControl::update()` (`2.5 s` timeout) heartbeats, ensuring the hardware monostable (`U4` `SN74LVC1G123`) cuts heater power if either the main loop or control loop stalls.
- **NTP & WiFi Watchdog**: Added `og3::Ntp` time synchronization and `og3::WifiWatchdog` recovery.
- **MQTT Mode & SSL/TLS Configuration**: Exposed MQTT broker mode (`Home Assistant` / `Adafruit IO`) and SSL/TLS certificate verification settings in the Svelte web UI.

### Changed
- **Library Upgrades**: Migrated firmware to `chl33/og3@^0.7.0` and pinned `platform = espressif32@^7.1.3` and updated sensor/display dependencies.
- **Svelte Build & Asset Caching**: Upgraded `svelteesp32` to `3.2.5` with `--etag=always` for HTTP ETag caching.
- **Immediate MQTT State Sync**: Published state and persisted target temperature immediately when fan mode or target temperature is updated via MQTT.

### Fixed
- **Home Assistant Climate Discovery Templates**: Updated MQTT discovery value templates to match `camelCase` variable keys (`heatMode`, `setTemp`, `fanMode`).
- **WiFi Configuration Reboot**: Added automatic device reboot after saving updated WiFi settings via `/api/wifi`.

## [1.0.0] - 2026-03-29

### Added
- New responsive Svelte web interface for status monitoring and configuration.
- JSON API for full device control and status reporting.
- Standardized `camelCase` naming for all API keys and internal variables.
- Manual fan control via web UI.
- Integrated "Run Test" command button in configuration page.
- Switch to `min_spiffs.csv` partition table to support OTA updates with the new web assets.

### Changed
- Reorganized web server routes (legacy HTML interface moved to `/old`).
- Improved PID control stability and configuration.
- Standardized variable naming across firmware and frontend.
