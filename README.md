# Matter-over-Wi-Fi LED Ecosystem

A scalable Matter-compatible LED control system for ESP32 variants (S3, C3, C6) supporting addressable LED strips, per-segment effects, and multi-device orchestration.

## Features

- **Matter Protocol Support**: Full Matter compliance for multi-ecosystem control (Apple Home, Google Home, Amazon Alexa)
- **Addressable LED Strips**: WS2812/SK6812 support with per-segment control
- **Effect Engine**: Built-in effects (solid, rainbow, breathing, sparkle, chase, music sync)
- **Hardware Abstraction**: Portable across ESP32-S3, C3, and C6 variants
- **Temperature Sensor**: Exposes internal chip temperature as a Matter endpoint
- **Persistent Storage**: Saves LED state across reboots via NVS

## Requirements

- ESP-IDF v5.1.4+
- esp-matter (in `esp-matter-v2/` directory)
- Python 3.8+
- ESP32-S3, ESP32-C3, or ESP32-C6 development board
- WS2812 LED strip

## Project Structure

```
matter-over-wifi/
├── firmware/                   # Main firmware application
│   ├── CMakeLists.txt          # Project-level CMake config
│   ├── partitions.csv          # Flash partition table
│   └── main/                   # Application source code
│       ├── main.cpp            # Entry point, init, main loop
│       ├── config/             # Board & Matter configuration
│       │   └── board_config.h
│       ├── hal/                # Hardware abstraction layer
│       │   ├── hal_interface.h # Common HAL interface
│       │   ├── hal_esp32s3.cpp
│       │   ├── hal_esp32c3.cpp
│       │   └── hal_esp32c6.cpp
│       ├── led_engine/         # LED effect engine
│       │   ├── led_engine.h
│       │   ├── led_engine.cpp
│       │   ├── music_sync.h
│       │   └── music_sync.cpp
│       ├── matter/             # Matter protocol integration
│       │   ├── matter_device.h
│       │   └── matter_device.cpp
│       ├── network/            # Wi-Fi management
│       │   ├── network_manager.h
│       │   └── network_manager.cpp
│       ├── storage/            # NVS persistence
│       │   ├── storage_manager.h
│       │   └── storage_manager.cpp
│       └── util/               # Logging utilities
│           └── logging.h
├── esp-idf/                    # ESP-IDF SDK (v5.1)
├── esp-matter-v2/              # esp-matter SDK
├── test/                       # Unit tests
├── docs/                       # Documentation
└── tools/                      # Build and flash scripts
```

## Quick Start

### 1. Set Up Environment (PowerShell)

```powershell
# Using full path
.\build_firmware.ps1

# Or using C:\mow junction (if set up)
.\build_firmware_junc.ps1
```

### 2. Manual Build

```powershell
# Set environment variables
$env:IDF_PATH = "C:\Users\patij212\Downloads\matter-over-wifi\esp-idf"
$env:ESP_MATTER_PATH = "C:\Users\patij212\Downloads\matter-over-wifi\esp-matter-v2"

# Export ESP-IDF tools
. .\esp-idf\export.ps1

# Build
cd firmware
idf.py set-target esp32s3   # or esp32c3, esp32c6
idf.py build
```

### 3. Flash to Device

```powershell
idf.py -p COM<X> flash monitor
```

### 4. Commission via Matter

1. Open a Matter-compatible app (Apple Home, Google Home, etc.)
2. Scan the QR code displayed in the serial monitor
3. Complete the commissioning process

## Configuration

### LED Strip Configuration

Edit `firmware/main/config/board_config.h`:

```c
#define LED_STRIP_COUNT     60    // Number of LEDs in strip
#define LED_REFRESH_RATE_HZ 60    // Frame rate
```

GPIO pins are auto-selected per target (S3=GPIO48, C3/C6=GPIO8).

### Building for Different Targets

```powershell
cd firmware
idf.py set-target esp32c3  # For ESP32-C3
idf.py set-target esp32c6  # For ESP32-C6
idf.py set-target esp32s3  # For ESP32-S3
idf.py build
```

## Architecture

The firmware uses a layered architecture:

- **HAL Layer**: Hardware-specific GPIO, LED strip (RMT), button, temp sensor implementations
- **LED Engine**: Segment-based effect rendering with gamma correction and thread safety
- **Matter Layer**: esp-matter SDK integration with On/Off, Level, Color Control, and Temperature clusters
- **Network Layer**: Wi-Fi STA management with auto-reconnect
- **Storage Layer**: NVS-based state persistence

Matter endpoints:
- Endpoint 1: Extended Color Light (LED strip control)
- Endpoint 2: Temperature Sensor (chip temperature)

## License

MIT License - See LICENSE file for details
