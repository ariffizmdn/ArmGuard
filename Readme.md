# armguard-wearable-safety
# ArmGuard — IoT Wearable Safety & Localization System for Oil Palm Plantation Workers

An edge-computing IoT safety wearable built for the **Integrated Design Project (BERN 3863)** course at **Universiti Teknikal Malaysia Melaka (UTeM)**.

## What It Does
- **Detects Worker Falls**: Uses a two-stage IMU algorithm to detect free-fall and sudden impacts.
- **Monitors Heart Rate**: Tracks worker vitals in real-time to eliminate false positives and catch sudden health emergencies.
- **GPS Emergency Localization**: Activates power-gated GPS module only during alerts to pinpoint injured workers in dense plantations.
- **Dual-Tier Alerts**: Instantly pushes real-time location and status to a **Web Supervisor Dashboard** and **Telegram Bot**.

## How It Works
- **Microcontroller**: Seeed Studio XIAO ESP32-C3 (Edge data filtering & multi-sensor fusion)
- **Kinematic & Vital Sensors**: MPU-6050 IMU (6-axis movement) + PPG Pulse Sensor
- **Geospatial & Power Management**: NEO-6M GPS module powered through an SI2301 P-Channel MOSFET circuit (isolated during sleep to reduce current to **1.34 mA**)
- **Cloud & Alerts**: Firebase Realtime Database + Node.js Telegram Bot

## Files in This Repo
- `firmware/` — ESP32-C3 C++ code & configuration files
- `backend/` — Node.js script for Telegram Bot alerts
- `web-dashboard/` — HTML/JS supervisor dashboard with live map
- `hardware/` — Schematic diagrams & bill of materials (BOM)
- `TECHNICAL REPORT IDP02.pdf` — Complete project technical report

## Quick Test Results

| Test Parameter | Result / Metric | Notes |
|---|---|---|
| **Resting Heart Rate Accuracy** | **97.57%** (2.43% Error) | Benchmark against Huawei GT3 Pro |
| **Post-Exercise HR Accuracy** | **97.61%** (2.39% Error) | Measured after physical exertion |
| **Fall Detection Reliability** | **85% – 95%** Sensitivity | Tested across 20 simulated fall scenarios |
| **Baseline Power Consumption** | **1.34 mA** (Sleep Mode) | Enables up to 96+ hours on a 1500 mAh LiPo |

Feel free to clone, build, or improve it!  
Designed to help protect high-risk lone workers in dense agricultural environments.

---
*Open source for educational and safety research use*
