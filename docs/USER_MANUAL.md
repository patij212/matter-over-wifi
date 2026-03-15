# Matter LED Ecosystem - User Manual

## Overview

The Matter LED Ecosystem is a smart LED strip controller built on ESP32. It supports the Matter protocol over Wi-Fi, allowing control from Apple Home, Google Home, Amazon Alexa, Samsung SmartThings, and any other Matter-compatible platform.

**Features:**
- WS2812/SK6812 addressable LED strip control (up to 60 LEDs)
- 10 built-in lighting effects
- Full RGB color and brightness control via Matter
- On-chip temperature sensor exposed as a Matter endpoint
- Music-reactive mode (ESP32-S3 only)
- State persistence across power cycles (NVS)
- OTA-ready dual partition layout
- Button for toggle and factory reset

**Supported Hardware:**

| Board | Chip | BLE | Wi-Fi | Music Sync | Status LED |
|-------|------|-----|-------|------------|------------|
| ESP32-S3 DevKit | ESP32-S3 | BLE 5.0 (NimBLE) | 802.11 b/g/n | Yes (ADC on GPIO1) | GPIO 2 |
| ESP32-C3 DevKit | ESP32-C3 | BLE 5.0 (NimBLE) | 802.11 b/g/n | No | None |
| ESP32-C6 DevKit | ESP32-C6 | BLE 5.0 (NimBLE) | 802.11 b/g/n/ax | No | None |

---

## 1. Getting Started

### 1.1 What You Need

**Required:**
- 1x ESP32-S3, ESP32-C3, or ESP32-C6 development board
- 1x WS2812B LED strip (up to 60 LEDs recommended)
- 1x 5V power supply (sized for your LED count - see Power section)
- Hookup wire (22-26 AWG)
- USB-C cable (for flashing)

**Optional:**
- 1x 330-470 ohm resistor (data line protection)
- 1x 1000uF electrolytic capacitor (power smoothing)
- 1x MAX4466 or similar electret mic amplifier module (music sync, S3 only)
- 1x Breadboard or perfboard

### 1.2 Flashing the Firmware

1. Connect the ESP32 board via USB
2. Open a terminal in the `firmware/` directory
3. Run the build and flash:
   ```
   idf.py build
   idf.py -p COMx flash monitor
   ```
   Replace `COMx` with your serial port (e.g., `COM3` on Windows, `/dev/ttyUSB0` on Linux)

4. The serial monitor will show boot messages including the QR code payload for commissioning.

### 1.3 Commissioning (First-Time Setup)

When the device boots for the first time (or after a factory reset), it enters **commissioning mode**:

1. The LED strip shows **blue breathing** to indicate it is waiting to be paired
2. The device advertises via BLE
3. Open your Matter controller app:
   - **Apple Home**: Tap "+", "Add Accessory", scan QR code
   - **Google Home**: Tap "+", "Set up device", "New device", scan QR code
   - **Amazon Alexa**: Alexa app > Devices > "+" > "Add Device" > "Other" > Matter
4. The QR code and manual pairing code are printed to the serial console at boot

**Default Pairing Credentials:**

| Parameter | Value |
|-----------|-------|
| Discriminator | 3840 |
| Passcode | 20202021 |
| Vendor ID | 0xFFF1 (test) |
| Product ID | 0x8001 |

> **Note:** The passcode `20202021` is a development default. For production, generate unique credentials.

### 1.4 After Commissioning

Once commissioned:
- Wi-Fi credentials are stored in NVS (non-volatile storage)
- The device connects to your Wi-Fi network automatically on every boot
- The LED strip returns to its last saved color/effect
- You can control it from any Matter controller on your network

---

## 2. Matter Endpoints

The device exposes two Matter endpoints:

| Endpoint | Type | Clusters | Description |
|----------|------|----------|-------------|
| 1 | Extended Color Light | On/Off, Level Control, Color Control | Controls the LED strip |
| 2 | Temperature Sensor | Temperature Measurement | Reports chip temperature |

### 2.1 Light Control (Endpoint 1)

From your Matter controller you can:
- **On/Off** - Toggle the LED strip
- **Brightness** - Set brightness (0-254, mapped to 0-255 internally)
- **Color** - Set hue and saturation (HSV mode)

### 2.2 Temperature Sensor (Endpoint 2)

Reports the ESP32's internal chip temperature every 10 seconds. Range: -10.0 to 80.0 degrees C. This appears as a separate temperature sensor accessory in your smart home app.

---

## 3. LED Effects

The firmware includes 10 built-in effects:

| # | Effect | Description |
|---|--------|-------------|
| 0 | **Solid** | Static single color |
| 1 | **Breathing** | Smooth pulsing brightness (sine wave) |
| 2 | **Rainbow** | All LEDs cycle through the color wheel together |
| 3 | **Rainbow Wave** | Moving rainbow gradient across the strip |
| 4 | **Sparkle** | Random white sparkles over a base color |
| 5 | **Fire** | Warm fire simulation with random flicker |
| 6 | **Chase** | 3-pixel dot chasing along the strip |
| 7 | **Strobe** | On/off flash |
| 8 | **Gradient** | Static gradient from primary to secondary color |
| 9 | **Music Sync** | Audio-reactive (ESP32-S3 only, requires mic) |

Effects are rendered at 60 FPS. Speed is adjustable per segment.

---

## 4. Button Controls

The BOOT button on the development board provides two functions:

| Action | Duration | Function |
|--------|----------|----------|
| Short press | < 10 seconds | Toggle LED strip on/off |
| Long press | >= 10 seconds | Factory reset (erases all data, restarts) |

**Button GPIO:** GPIO 0 (S3) / GPIO 9 (C3, C6)

> **Warning:** Factory reset erases Wi-Fi credentials, Matter fabric data, and saved LED state. You will need to re-commission the device.

---

## 5. Power Management

When `CONFIG_PM_ENABLE` is set in sdkconfig, the firmware enables:
- **Dynamic Frequency Scaling (DFS)**: CPU scales between 80-240 MHz based on load
- **Automatic Light Sleep**: CPU enters light sleep during idle periods

This reduces power consumption when the LEDs are off or running a low-activity effect.

---

## 6. Troubleshooting

| Symptom | Cause | Solution |
|---------|-------|----------|
| LEDs don't light up | Wrong GPIO or no power | Check wiring (see Wiring Guide) |
| Blue breathing but won't pair | BLE range or controller issue | Move closer, restart controller app |
| Paired but no response | Wi-Fi issue | Check router, ensure 2.4GHz network |
| Colors look wrong | GRB vs RGB mismatch | WS2812B uses GRB order (handled by driver) |
| Device offline after power cycle | Wi-Fi reconnecting | Wait 10-15 seconds after boot |
| Flashing/flickering LEDs | Insufficient power supply | Use a beefier 5V PSU, add capacitor |
| Factory reset not working | Button not held long enough | Hold BOOT button for 10+ seconds |

### Serial Debug

Connect via USB and open a serial monitor at **115200 baud** to see debug logs:
```
idf.py -p COMx monitor
```

Log tags: `[MATTER]`, `[LED]`, `[HAL_S3]`, `[STORAGE]`, `[NETWORK]`

---

## 7. Updating Firmware

The partition layout supports OTA updates:

| Partition | Size | Purpose |
|-----------|------|---------|
| ota_0 | 1800 KB | App slot A |
| ota_1 | 1800 KB | App slot B |
| nvs | 24 KB | Settings storage |
| otadata | 8 KB | OTA boot selector |

To flash manually via USB:
```
idf.py -p COMx flash
```

OTA over Matter is planned but not yet implemented.
