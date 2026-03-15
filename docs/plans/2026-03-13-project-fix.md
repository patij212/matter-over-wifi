# Project Fix Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Fix the broken Matter-over-WiFi LED ecosystem project to have a clean, correct structure that builds against esp-matter-v2.

**Architecture:** Keep all working source code (HAL, LED engine, Matter device, storage, network). Rewrite CMakeLists.txt files to match actual file layout and use correct esp-matter-v2 paths (modeled after esp-matter-v2/examples/light/). Remove empty placeholder stubs. Add missing ESP32-C6 HAL. Fix build scripts.

**Tech Stack:** ESP-IDF v5.1, esp-matter-v2, C++17, CMake, FreeRTOS

---

### Task 1: Remove empty placeholder files

**Files:**
- Delete: `firmware/main/app_task.cpp`
- Delete: `firmware/main/device_logic.cpp`
- Delete: `firmware/main/matter_callbacks.cpp`
- Delete: `firmware/main/drivers/led_driver.cpp`
- Delete: `firmware/main/drivers/ws2812_driver.cpp`
- Delete: `firmware/main/matter/clusters/on_off_cluster.cpp`
- Delete: `firmware/main/matter/clusters/level_cluster.cpp`
- Delete: `firmware/main/matter/clusters/color_cluster.cpp`

These are all empty stubs with just a TAG variable. All actual functionality is already in matter_device.cpp, led_engine.cpp, and the HAL files.

### Task 2: Remove old broken esp-matter directory

- Delete: `esp-matter/` (already gutted by rm_esp_matter.py)
- Delete: `rm_esp_matter.py` (no longer needed)

### Task 3: Clean up doc extraction artifacts

- Delete: `docx_extracted/`
- Delete: `docx_temp.zip`
- Delete: `document_content.txt`

### Task 4: Rewrite firmware/CMakeLists.txt

Model after esp-matter-v2/examples/light/CMakeLists.txt with correct paths.

### Task 5: Rewrite firmware/main/CMakeLists.txt

List only files that actually exist. Use SRC_DIRS approach or explicit SRCS.

### Task 6: Add ESP32-C6 HAL implementation

Copy from hal_esp32c3.cpp (identical RMT/GPIO/temp sensor API on C6).

### Task 7: Add music_sync.h header

The led_engine.cpp forward-declares music_sync functions. Create a proper header.

### Task 8: Fix build scripts

Point to esp-matter-v2 with correct connectedhomeip path.

### Task 9: Update .gitignore

Add esp-matter-v2, gn_out, build artifacts, and other generated content.

### Task 10: Fix .vscode/settings.json

Point to correct firmware directory for CMake.

### Task 11: Update tests

Fix test to match actual code structure.

### Task 12: Update README.md

Reflect actual project state and correct build instructions.
