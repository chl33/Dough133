# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
