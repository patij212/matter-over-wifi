# Wiring Guide - Matter LED Ecosystem

## Table of Contents

1. [Safety Warnings](#1-safety-warnings)
2. [Understanding Your Components](#2-understanding-your-components)
3. [Tools and Materials](#3-tools-and-materials)
4. [Power Planning](#4-power-planning)
5. [Pin Reference](#5-pin-reference)
6. [Wiring Diagram: ESP32-S3 Full Build](#6-wiring-diagram-esp32-s3-full-build)
7. [Wiring Diagram: ESP32-C3](#7-wiring-diagram-esp32-c3)
8. [Wiring Diagram: ESP32-C6](#8-wiring-diagram-esp32-c6)
9. [Step-by-Step: Basic LED Strip Connection](#9-step-by-step-basic-led-strip-connection)
10. [Step-by-Step: External Power Supply](#10-step-by-step-external-power-supply)
11. [Step-by-Step: Power Injection for Long Strips](#11-step-by-step-power-injection-for-long-strips)
12. [Step-by-Step: Music Sync Microphone (S3 Only)](#12-step-by-step-music-sync-microphone-s3-only)
13. [Optional: Level Shifter](#13-optional-level-shifter)
14. [Verification Checklist](#14-verification-checklist)
15. [Troubleshooting Wiring Issues](#15-troubleshooting-wiring-issues)
16. [Common Mistakes](#16-common-mistakes)
17. [Bill of Materials](#17-bill-of-materials)

---

## 1. Safety Warnings

**Read these before wiring anything.**

- **Disconnect power before making any wiring changes.** Never connect or disconnect wires while the power supply or USB cable is plugged in.
- **Double-check polarity.** Reversing 5V and GND on the LED strip will destroy the LEDs permanently.
- **Do not exceed voltage ratings.** The ESP32's GPIO pins are 3.3V logic. Feeding 5V directly into a GPIO pin (except the designated 5V pin) will permanently damage the chip.
- **Do not power more than ~10 LEDs from the ESP32's USB port.** The onboard voltage regulator and USB port cannot supply enough current. You will get brownouts, resets, or damage the board.
- **Use appropriately rated wire.** For 60 LEDs at full white (3.6A), use at least 22 AWG wire for 5V and GND runs. For longer runs (>50cm), use 20 AWG or thicker.
- **Electrolytic capacitors have polarity.** The longer leg is positive (+). The striped side is negative (-). Reversing them can cause failure or leaking.
- **Keep data wires short.** The WS2812B data signal is sensitive to interference. Keep the wire between the ESP32 and the first LED under 30cm (12 inches). Longer runs require a resistor or level shifter.

---

## 2. Understanding Your Components

### 2.1 WS2812B LED Strip

The WS2812B is an addressable RGB LED with an integrated controller. Each LED has 4 pins, but strips expose 3 pads/wires:

```
    WS2812B Strip (typical 3-wire)
    ============================================================

    Wire Colors (vary by manufacturer):

    Pad Label   Common Wire Colors         Function
    ---------   ----------------------     -------------------------
    5V / VCC    Red                        +5V Power Input
    DIN / DI    Green, Yellow, or White    Data Input (from controller)
    GND / VSS   White, Black, or Blue      Ground

    DOUT / DO   (connects to next LED's DIN internally in the strip)


    Data flows in one direction:

      DIN                                                     DOUT
       |                                                       |
       v                                                       v
    [LED 0] ---> [LED 1] ---> [LED 2] ---> ... ---> [LED 59] --->
       ^
       |
    Connect your ESP32 data pin HERE (at the DIN end)
```

**How to identify the DIN (input) end:**
- Look for arrows printed on the strip pointing away from the input end
- The input end is often marked "DIN" or "DI"
- The output end is marked "DOUT" or "DO"
- If you connect to the wrong end, no LEDs will light up (data flows one direction only)

### 2.2 ESP32 Development Board

Your dev board provides pin headers along both edges. Key pins used in this project:

```
    Typical ESP32-S3-DevKitC-1 (top view, USB port facing up)
    +------[USB-C]------+
    |                    |
    | [3V3]        [3V3] |    <-- 3.3V output (for mic module)
    | [3V3]         [RST] |
    | [GND]         [36]  |
    | [ 4 ]         [35]  |
    | [ 5 ]         [34]  |
    | [ 6 ]         [33]  |
    | [ 7 ]         [26]  |
    | [15 ]         [25]  |
    | [16 ]         [21]  |
    | [17 ]         [20]  |    <-- USB D+
    | [18 ]         [19]  |    <-- USB D-
    | [ 8 ]         [GND] |
    | [ 3 ]         [48]  |    <-- LED STRIP DATA
    | [46 ]         [47]  |
    | [ 9 ]         [GND] |
    | [10 ]         [GND] |
    | [11 ]         [ 2]  |    <-- Status LED
    | [12 ]         [ 1]  |    <-- Audio Input (ADC)
    | [13 ]         [GND] |
    | [14 ]         [ 0]  |    <-- BOOT Button
    | [ 5V]         [GND] |
    | [GND]         [GND] |
    |                    |
    +--------------------+

    NOTE: Pin layout varies between board manufacturers.
          Always verify with YOUR board's datasheet/silkscreen.
```

### 2.3 Power Supply

Use a regulated 5V DC power supply. Common options:

| Type | Pros | Cons | Best For |
|------|------|------|----------|
| USB charger (5V 2A) | Cheap, easy | Limited current | 1-15 LEDs |
| 5V 3A wall adapter | Barrel jack, common | Moderate current | 15-30 LEDs |
| 5V 5A switching PSU | High current, reliable | Larger, needs wiring | 30-60 LEDs |
| 5V 10A PSU | Very high current | Bulky, overkill for 60 LEDs | 60+ LEDs |

> Look for power supplies with short-circuit and overcurrent protection. Avoid no-name unregulated supplies.

---

## 3. Tools and Materials

### Required Tools
- Soldering iron and solder (for permanent connections) OR breadboard and jumper wires (for prototyping)
- Wire strippers
- Multimeter (highly recommended for verifying connections)
- Small Phillips screwdriver (for screw-terminal PSU)

### Wire Color Convention Used in This Guide

| Color | Function |
|-------|----------|
| **Red** | +5V power |
| **Black** | Ground (GND) |
| **Green** | Data signal (GPIO to LED DIN) |
| **Blue** | Audio signal (mic module to ADC) |
| **Orange** | 3.3V power (for mic module) |

You can use any wire colors you have. This convention is just for clarity in the diagrams below.

---

## 4. Power Planning

### 4.1 Current Draw Reference

| Component | Current Draw | Notes |
|-----------|-------------|-------|
| Single WS2812B LED (full white, 100% brightness) | 60 mA | All 3 channels on max |
| Single WS2812B LED (single color, 100% brightness) | 20 mA | Only 1 channel on |
| Single WS2812B LED (typical mixed use) | ~30 mA | Average in practice |
| ESP32-S3 (Wi-Fi + BLE active) | ~240 mA | Peak during commissioning |
| ESP32-C3 (Wi-Fi + BLE active) | ~170 mA | |
| ESP32-C6 (Wi-Fi + BLE active) | ~170 mA | |
| MAX4466 mic module | ~0.5 mA | Negligible |

### 4.2 Power Budget Calculator

**Worst-case formula:**
```
Total Amps = (LED_COUNT x 0.060) + 0.250
```

**Realistic formula** (not all LEDs at full white simultaneously):
```
Total Amps = (LED_COUNT x 0.030) + 0.250
```

| LED Count | Worst Case | Realistic | Recommended PSU |
|-----------|-----------|-----------|-----------------|
| 1-10 | 0.85 A | 0.55 A | USB only (no external PSU needed) |
| 11-20 | 1.45 A | 0.85 A | 5V 2A |
| 21-30 | 2.05 A | 1.15 A | 5V 3A |
| 31-45 | 2.95 A | 1.60 A | 5V 3A |
| 46-60 | 3.85 A | 2.05 A | 5V 5A |

### 4.3 Power Supply Sizing Rule

Always choose a power supply rated for at least **120% of your worst-case current**. This gives headroom and keeps the supply running cool.

Example: 60 LEDs worst case = 3.85A. 120% = 4.62A. Use a **5V 5A** supply.

### 4.4 Voltage Drop on Long Strips

WS2812B strips have thin copper traces. Over long runs, resistance causes the voltage to drop, making LEDs at the far end appear dimmer or shift color (usually toward red/orange, since blue requires the most voltage).

| Strip Length | Symptoms | Solution |
|-------------|----------|----------|
| Under 30 LEDs | Usually fine | Single-end power is OK |
| 30-60 LEDs | Slight dimming at far end | Inject power at both ends |
| 60+ LEDs | Noticeable color shift, dimming | Inject power every 30-50 LEDs |

---

## 5. Pin Reference

### 5.1 ESP32-S3

```
    .-----------------------------------------------.
    | ESP32-S3 Pin Assignments                      |
    |-----------------------------------------------|
    | Pin      | Direction | Function               |
    |----------|-----------|------------------------|
    | GPIO 48  | OUTPUT    | LED Strip Data (DIN)   |
    | GPIO 0   | INPUT     | BOOT Button (on-board) |
    | GPIO 2   | OUTPUT    | Status LED (on-board)  |
    | GPIO 1   | INPUT     | Audio ADC (mic module)  |
    | 5V       | POWER     | 5V output (from USB)   |
    | 3V3      | POWER     | 3.3V output (for mic)  |
    | GND      | POWER     | Ground (multiple pins) |
    '-----------------------------------------------'
```

### 5.2 ESP32-C3

```
    .-----------------------------------------------.
    | ESP32-C3 Pin Assignments                      |
    |-----------------------------------------------|
    | Pin      | Direction | Function               |
    |----------|-----------|------------------------|
    | GPIO 8   | OUTPUT    | LED Strip Data (DIN)   |
    | GPIO 9   | INPUT     | BOOT Button (on-board) |
    | 5V       | POWER     | 5V output (from USB)   |
    | 3V3      | POWER     | 3.3V output            |
    | GND      | POWER     | Ground (multiple pins) |
    '-----------------------------------------------'
    Note: No audio input - music sync not supported.
```

### 5.3 ESP32-C6

```
    .-----------------------------------------------.
    | ESP32-C6 Pin Assignments                      |
    |-----------------------------------------------|
    | Pin      | Direction | Function               |
    |----------|-----------|------------------------|
    | GPIO 8   | OUTPUT    | LED Strip Data (DIN)   |
    | GPIO 9   | INPUT     | BOOT Button (on-board) |
    | 5V       | POWER     | 5V output (from USB)   |
    | 3V3      | POWER     | 3.3V output            |
    | GND      | POWER     | Ground (multiple pins) |
    '-----------------------------------------------'
    Note: No audio input - music sync not supported.
```

---

## 6. Wiring Diagram: ESP32-S3 Full Build

This diagram shows the complete wiring for an ESP32-S3 with a 60-LED strip, external power supply, protection components, and music sync microphone.

```
                          USB Cable
                            | |
                       +----+ +----+
                       |  USB-C    |
                       |  (power   |
                       |  & flash) |
    +==================|===========|==================+
    |                  ESP32-S3 DevKit                |
    |                                                 |
    |  [3V3]  [GND]  [GPIO 48]  [GPIO 1]  [5V]      |
    +====+======+========+==========+========+========+
         |      |        |          |        |
         |      |        |          |        |
  ORANGE |      | BLACK  | GREEN    | BLUE   | (not used for
  wire   |      | wire   | wire     | wire   |  external power)
         |      |        |          |
         |      |        |          |
         |      +--------+-----+   |     +==============+
         |      |        |     |   |     |  5V PSU      |
         |      |        |     |   |     |  (5A)        |
         |      |        |     |   |     |              |
         |      |        |     |   |     | [+5V] [GND]  |
         |      |        |     |   |     +===+=====+=====
         |      |        |     |   |         |     |
         |      |        |     |   |    RED  |     | BLACK
         |      |        |     |   |    wire |     | wire
         |      |        |     |   |         |     |
         |      |        |     |   |   +-----+     |
         |      |        |     |   |   |     +-----+----+
         |      |        |     |   |   |           |    |
         |      |        |     |   |   |  +========+==  |  1000uF 25V
         |      |        |     |   |   |  || (+)      |  |  Capacitor
         |      |        |     |   |   |  ||  CAP     |  |  (between
         |      |        |     |   |   |  || (-)      |  |   +5V & GND
         |      |        |     |   |   |  +========+==  |   at strip)
         |      |        |     |   |   |           |    |
         |      |    +---+     |   |   |           |    |
         |      |    |         |   |   |           |    |
         |      |    | [330R]  |   |   |           |    |
         |      |    |  ===    |   |   |           |    |
         |      |    +---+     |   |   |           |    |
         |      |        |     |   |   |           |    |
         |      |        v     |   |   v           |    v
         |      |  +=====+=====+===+===+===========+====+=====+
         |      |  ||                                          ||
         |      |  ||   DIN     5V     GND    WS2812B Strip   ||
         |      |  ||   (grn)  (red)  (blk)                   ||
         |      |  ||                                          ||
         |      |  || [LED 0][LED 1][LED 2]  ...  [LED 59]    ||
         |      |  ||                                          ||
         |      |  ||           Arrow direction --->           ||
         |      |  +============================================+
         |      |
         |      |     +-----------+
         |      |     |  MAX4466  |
         |      |     |  Mic Amp  |
         |      |     |           |
         +------|---->| VCC       |
                |     |           |
                +---->| GND       |
                      |           |
           BLUE wire  |    OUT  --+  (connects to GPIO 1
                      |           |   on the ESP32 above)
                      +-----------+

    WIRE SUMMARY:
    +-------+--------+-------------------+-------------------+
    | Wire  | Color  | From              | To                |
    +-------+--------+-------------------+-------------------+
    | 1     | GREEN  | ESP32 GPIO 48     | 330R -> Strip DIN |
    | 2     | RED    | PSU +5V           | Strip 5V          |
    | 3     | BLACK  | PSU GND           | Strip GND         |
    | 4     | BLACK  | ESP32 GND         | Strip GND (same)  |
    | 5     | ORANGE | ESP32 3V3         | MAX4466 VCC       |
    | 6     | BLACK  | ESP32 GND         | MAX4466 GND       |
    | 7     | BLUE   | MAX4466 OUT       | ESP32 GPIO 1      |
    +-------+--------+-------------------+-------------------+
    + 1000uF capacitor across Strip 5V and Strip GND
    + 330 ohm resistor in series with data line
```

---

## 7. Wiring Diagram: ESP32-C3

Simpler build: no music sync, no status LED.

```
                          USB Cable
                            | |
                       +----+ +----+
                       |  USB-C    |
    +==================|===========|==================+
    |                  ESP32-C3 DevKit                |
    |                                                 |
    |              [GND]  [GPIO 8]  [5V]              |
    +================+========+=======+===============+
                     |        |       |
                     |        |       |
               BLACK |  GREEN |       | (not used for
               wire  |  wire  |       |  external power)
                     |        |
                     |   +----+
                     |   |
                     |   | [330R]     +==============+
                     |   |  ===       |  5V PSU      |
                     |   +----+       |  (3-5A)      |
                     |        |       |              |
                     |        |       | [+5V] [GND]  |
                     |        |       +===+======+===+
                     |        |           |      |
                     |        |      RED  |      | BLACK
                     |        |      wire |      | wire
                     |        |           |      |
                     |        |     +-----+      |
                     +--------+--+  |    +-------+
                              |  |  |    |
                              v  |  v    v
    +=========================+==+==+====+==============+
    ||                                                  ||
    ||   DIN     5V     GND       WS2812B Strip        ||
    ||   (grn)  (red)  (blk)                           ||
    ||                                                  ||
    || [LED 0][LED 1][LED 2]  ...  [LED 59]            ||
    ||                                                  ||
    ||           Arrow direction --->                   ||
    +====================================================+

    WIRE SUMMARY:
    +-------+--------+-------------------+-------------------+
    | Wire  | Color  | From              | To                |
    +-------+--------+-------------------+-------------------+
    | 1     | GREEN  | ESP32 GPIO 8      | 330R -> Strip DIN |
    | 2     | RED    | PSU +5V           | Strip 5V          |
    | 3     | BLACK  | PSU GND           | Strip GND         |
    | 4     | BLACK  | ESP32 GND         | Strip GND (same)  |
    +-------+--------+-------------------+-------------------+
    + Optional: 1000uF capacitor across Strip 5V and Strip GND
    + 330 ohm resistor in series with data line
```

---

## 8. Wiring Diagram: ESP32-C6

Identical to the C3 wiring. The ESP32-C6 uses the same GPIO 8 for data and GPIO 9 for the boot button.

```
                          USB Cable
                            | |
                       +----+ +----+
                       |  USB-C    |
    +==================|===========|==================+
    |                  ESP32-C6 DevKit                |
    |                                                 |
    |              [GND]  [GPIO 8]  [5V]              |
    +================+========+=======+===============+
                     |        |       |
                     |        |       |
               BLACK |  GREEN |       | (not used for
               wire  |  wire  |       |  external power)
                     |        |
                     |   +----+
                     |   |
                     |   | [330R]     +==============+
                     |   |  ===       |  5V PSU      |
                     |   +----+       |  (3-5A)      |
                     |        |       |              |
                     |        |       | [+5V] [GND]  |
                     |        |       +===+======+===+
                     |        |           |      |
                     |        |      RED  |      | BLACK
                     |        |      wire |      | wire
                     |        |           |      |
                     |        |     +-----+      |
                     +--------+--+  |    +-------+
                              |  |  |    |
                              v  |  v    v
    +=========================+==+==+====+==============+
    ||                                                  ||
    ||   DIN     5V     GND       WS2812B Strip        ||
    ||   (grn)  (red)  (blk)                           ||
    ||                                                  ||
    || [LED 0][LED 1][LED 2]  ...  [LED 59]            ||
    ||                                                  ||
    ||           Arrow direction --->                   ||
    +====================================================+

    Wiring is identical to ESP32-C3 (Section 7).
    The only hardware difference is Wi-Fi 6 (802.11ax) support.
```

---

## 9. Step-by-Step: Basic LED Strip Connection

This is the simplest possible setup: a short LED strip powered from USB.

**Suitable for: 1-10 LEDs, prototyping, testing**

### Parts Needed
- ESP32 dev board (any variant)
- WS2812B strip (1-10 LEDs)
- 1x 330 ohm resistor
- 3x jumper wires (or solder + hookup wire)

### Wiring Diagram

```
    ESP32 Dev Board                          WS2812B Strip (1-10 LEDs)
    (powered via USB)
    +------------------+                     +============================+
    |                  |                     ||                           ||
    |            GPIOx +---[330 ohm]-------->|| DIN                      ||
    |                  |    resistor         ||                           ||
    |              5V  +------ RED wire ---->|| 5V                       ||
    |                  |                     ||                           ||
    |             GND  +------ BLK wire ---->|| GND                      ||
    |                  |                     ||                           ||
    +------------------+                     || [0] [1] [2] ... [9]      ||
                                             ||                           ||
                                             || Arrow direction --->      ||
                                             +============================+

    GPIOx = GPIO 48 (S3) or GPIO 8 (C3/C6)
```

### Instructions

1. **Identify the DIN end of the strip.** Look for the arrow direction or "DIN" label. Data must enter from this end.

2. **Connect the data wire.**
   - Solder or clip a wire from **GPIO 48** (S3) or **GPIO 8** (C3/C6) on the ESP32.
   - Connect it through a **330 ohm resistor** to the **DIN** pad on the LED strip.
   - The resistor should be physically close to the strip end, not the ESP32 end.

3. **Connect power.**
   - Run a wire from the ESP32's **5V** pin to the strip's **5V** (red) pad.
   - Run a wire from the ESP32's **GND** pin to the strip's **GND** (white/black) pad.

4. **Plug in USB** to power the ESP32 and the strip simultaneously.

5. **Flash and test.** The strip should show blue breathing on first boot (commissioning mode).

> **Limitation:** USB can supply ~500mA (USB 2.0) to ~900mA (USB 3.0). At full white brightness, 10 LEDs draw 600mA + 240mA for the ESP32 = 840mA. Stay under 10 LEDs or reduce brightness.

---

## 10. Step-by-Step: External Power Supply

**Suitable for: 10-60 LEDs, permanent installations**

### Parts Needed
- Everything from the basic setup
- 5V power supply (3-5A, see Power Planning)
- 1x 1000uF 25V electrolytic capacitor (recommended)
- Additional hookup wire (18-22 AWG for power lines)

### Wiring Diagram

```
    +==================+          +================+
    |  5V Power Supply |          | ESP32 DevKit   |
    |                  |          | (USB-powered)  |
    | AC In    +5V GND |          |                |
    |  ~       [+] [-] |          | GPIOx     GND  |
    +======+====+===+==+          +===+========+===+
           |    |   |                 |        |
           |    |   |    330 ohm      |        |
           |    |   |    resistor     |        |
           |    |   |       +---[===]-+        |
           |    |   |       |                  |
           |    |   +-------+---------+--------+----+
           |    |           |         |             |
           |    +-----------+----+    |             |
           |                |    |    |             |
           |           +----+    |    |             |
           |           |         |    |             |
           |   1000uF  |         |    |             |
           |   CAP     |         |    |             |
           |  (+)||(-)  |         |    |             |
           |     ||     |         |    |             |
           |     +------+---------+    |             |
           |            |              |             |
           |            v              v             v
           |    +=======+=============++=============+=====+
           |    ||                                         ||
           |    ||  5V          DIN          GND           ||
           |    ||  (red)       (green)      (black)       ||
           |    ||                                         ||
           |    ||  [LED 0] [LED 1] [LED 2] ... [LED 59]  ||
           |    ||                                         ||
           |    ||  Arrow direction --->                   ||
           |    +===========================================+
           |
           | (AC mains - to wall outlet)


    IMPORTANT: Three GND connections must meet at the same point:
    - ESP32 GND
    - Power Supply GND (-)
    - LED Strip GND
    All three must be connected together. This is called "common ground."
```

### Instructions

1. **Disconnect everything.** No USB cable, no power supply plugged in.

2. **Wire the power supply to the LED strip.**
   - Connect PSU **+5V** (usually marked "V+" or "+5V") to the strip's **5V** pad using thick wire (20-22 AWG).
   - Connect PSU **GND** (usually marked "V-" or "GND" or "COM") to the strip's **GND** pad.

3. **Install the capacitor** (recommended).
   - Solder or connect a **1000uF 25V electrolytic capacitor** across the 5V and GND pads at the LED strip.
   - **Polarity matters:** The longer leg (or the side without the stripe) goes to **+5V**. The shorter leg (striped side) goes to **GND**.
   - This absorbs inrush current when LEDs switch on suddenly, preventing voltage dips.

   ```
       Strip 5V pad ----+----[======]----+---- Strip GND pad
                         |    1000uF     |
                         |   (+)   (-)   |
                         |               |
                    long leg         short leg
                    (no stripe)      (stripe side)
   ```

4. **Connect the ESP32 GND to the strip GND.**
   - Run a wire from any **GND** pin on the ESP32 to the strip's **GND** pad (or the same GND rail as the power supply).
   - **This is the most critical connection.** Without common ground, the data signal has no reference and the LEDs will not work or will display garbage.

5. **Connect the data line.**
   - Run a wire from **GPIO 48** (S3) or **GPIO 8** (C3/C6) through a **330 ohm resistor** to the strip's **DIN** pad.
   - Place the resistor close to the strip, not close to the ESP32.

6. **Do NOT connect the ESP32's 5V pin to anything.** The ESP32 gets its power from USB. The LED strip gets its power from the external PSU. They share only GND and the data line.

7. **Double-check all connections with a multimeter.**
   - Continuity between ESP32 GND and PSU GND (-): should beep.
   - No continuity between PSU +5V and ESP32 GPIO pins: should NOT beep.
   - Resistance from GPIO through resistor to strip DIN: should read ~330 ohms.

8. **Power on.** Plug in the external PSU first, then plug in the USB cable.

9. **Power off order.** Unplug USB first, then disconnect the PSU.

---

## 11. Step-by-Step: Power Injection for Long Strips

**Suitable for: 30+ LEDs where you see dimming or color shift at the far end**

The copper traces on LED strips have resistance. Over 30+ LEDs, the voltage at the far end drops below what the LEDs need, causing them to appear dimmer or shift toward red/orange.

The fix: inject 5V and GND at both ends (and optionally the middle) of the strip.

### Wiring Diagram

```
    5V Power Supply (5A+)
    +========================+
    | [+5V]           [GND]  |
    +===+=================+==+
        |                 |
        +-------+   +----+-------+
        |       |   |    |       |
        |       |   |    |       |
        v       |   v    |       v
    +===+===+   |   +=+==+   +===+===+
    | 5V    |   |   |GND |   | 5V    |
    | START |   |   |START|   | END   |
    +===+===+   |   +==+==+  +===+===+
        |       |      |         |
        v       v      v         v
    +===+===+===+======+=========+===========================+
    ||                                                       ||
    || 5V  DIN  GND                               5V   GND  ||
    ||                                                       ||
    || [LED 0] [LED 1] ...  [LED 29] [LED 30] ... [LED 59]  ||
    ||                                                       ||
    || Arrow direction --->                                  ||
    +=========================================================+
         ^
         |
    [330R]---GPIO 48/8 (ESP32)
         |
    ESP32 GND ---+--- (connects to strip GND at start)


    For strips >60 LEDs, add a THIRD injection point at the middle:

    +===+===+   +===+===+   +===+===+
    | 5V    |   | 5V    |   | 5V    |
    | START |   | MID   |   | END   |
    +---+---+   +---+---+   +---+---+
        |           |           |
    +===+===========+===========+==========================+
    || [LED 0] ... [LED 30] ... [LED 59] ...  [LED N]     ||
    +=======================================================+
```

### Instructions

1. **Wire the start of the strip** as in Section 10 (PSU +5V/GND to strip 5V/GND at the DIN end).

2. **Run additional 5V and GND wires to the far end** of the strip.
   - Solder or clip wires to the **5V** and **GND** pads at the DOUT (last LED) end.
   - Connect these wires back to the **same power supply's** +5V and GND terminals.

3. **Use appropriate wire gauge.** These power injection wires carry significant current. Use 20 AWG or thicker for runs over 50cm.

4. **Do NOT daisy-chain power supplies.** Use one single PSU for all injection points. Connecting multiple power supplies in parallel is dangerous unless they are specifically designed for it.

5. **Test brightness uniformity.** Set all LEDs to full white and check that the strip looks even from start to end. If the middle is still dim, add a middle injection point.

---

## 12. Step-by-Step: Music Sync Microphone (S3 Only)

**Applies to: ESP32-S3 only. The C3 and C6 do not have an audio ADC channel configured.**

The music sync feature uses an analog microphone amplifier module to pick up ambient sound and drive audio-reactive LED effects (bass pulses, mid-frequency color shifts, treble sparkles).

### Recommended Module

**MAX4466** electret microphone amplifier breakout:
- Operating voltage: 2.4V - 5.5V (use 3.3V with ESP32)
- Output: analog voltage centered at VCC/2
- Adjustable gain via onboard potentiometer
- Cost: ~$3-5

Alternatives: INMP441 (I2S digital, would need code changes), KY-037 (simpler but noisier).

### Wiring Diagram

```
    ESP32-S3 DevKit                    MAX4466 Module
    +------------------+               +------------------+
    |                  |   ORANGE wire  |                  |
    |            3V3  -+--------------->+ VCC              |
    |                  |               |                  |
    |                  |   BLUE wire   |                  |
    |         GPIO 1  -+<--------------+ OUT              |
    |                  |               |                  |
    |                  |   BLACK wire  |                  |
    |            GND  -+--------------->+ GND              |
    |                  |               |                  |
    +------------------+               | [pot] <- gain    |
                                       |   adjust screw   |
                                       |                  |
                                       | ((( mic )))      |
                                       +------------------+


    Connection Table:
    +------------------+-------------------+
    | MAX4466 Pin      | ESP32-S3 Pin      |
    +------------------+-------------------+
    | VCC              | 3V3 (3.3V)        |
    | GND              | GND               |
    | OUT              | GPIO 1 (ADC1_CH0) |
    +------------------+-------------------+
```

### Instructions

1. **Connect VCC to 3.3V** (NOT 5V).
   - The ESP32's ADC pins are rated for 0-3.3V input. If you power the MAX4466 from 5V, its output can reach up to 5V, which will **permanently damage GPIO 1**.
   - The MAX4466 works fine at 3.3V. Output will swing between 0V and 3.3V.

2. **Connect GND to GND.**

3. **Connect OUT to GPIO 1** (ADC1 Channel 0 on the S3).

4. **Adjust the gain potentiometer.**
   - Use a small screwdriver to turn the trimmer pot on the MAX4466 module.
   - **Goal:** In a quiet room, the output should sit at approximately 1.65V (half of 3.3V). With music playing at moderate volume, the signal should swing between ~0.3V and ~3.0V.
   - **Too much gain:** Signal clips at 0V and 3.3V, audio sounds distorted, bass detection is erratic.
   - **Too little gain:** Signal barely moves from 1.65V, LEDs don't react to music.
   - Start with the pot at mid-position and adjust while playing music at your normal volume.

5. **Placement tips:**
   - Point the microphone toward the sound source.
   - Keep the module away from the ESP32 and power supply to reduce electrical noise.
   - If you get noise/flickering without music, try adding a 10uF capacitor between VCC and GND on the module, or shorten the wires.

6. **Testing:** Set the LED effect to **Music Sync** (effect #9). Play music. You should see:
   - Bass beats cause brightness pulses
   - Mid frequencies shift the color hue
   - Treble/high-hats create white sparkles

---

## 13. Optional: Level Shifter

### When Do You Need One?

The ESP32 outputs 3.3V logic on its GPIO pins. The WS2812B data input expects a logic high of at least 0.7 x VDD = 0.7 x 5V = **3.5V**. This means 3.3V is technically below the guaranteed threshold.

**In practice, most WS2812B strips work fine with 3.3V logic**, especially with a short data wire and the 330 ohm series resistor. However, if you experience:
- First LED showing random colors or not responding
- Data corruption that gets worse with longer data wires
- Intermittent failures in cold temperatures

Then a level shifter will fix it.

### Wiring with SN74HCT125 Level Shifter

```
    ESP32                SN74HCT125N              LED Strip
    +------+            +----+----+               +-------+
    |      |            |  14| VCC|---[5V]        |       |
    | GPIO +---[330R]-->|1A  |    |               |       |
    |      |            |1Y  |----+--[330R]------>| DIN   |
    |      |            |    |    |               |       |
    |      |            | 1OE|---[GND]            |       |
    |      |            |    |    |               |       |
    | GND  +------------|GND | 7  |               | GND   |
    |      |            +----+----+               +-------+
    +------+

    SN74HCT125N Pin Connections:
    - Pin 14 (VCC): Connect to 5V
    - Pin 7  (GND): Connect to common GND
    - Pin 1  (1A):  Input from ESP32 GPIO (through 330R)
    - Pin 2  (1Y):  Output to LED strip DIN (through 330R)
    - Pin 3  (1OE): Connect to GND (active low enable)
    - All other pins: Leave unconnected (or tie unused OE pins to VCC)
```

Alternative chips: **74AHCT1G125**, **TXS0108E** (bidirectional, overkill but works).

---

## 14. Verification Checklist

Before powering on, verify every connection. Use a multimeter in continuity mode (beep mode).

### Pre-Power Checks

```
    [ ] Data line:
        GPIO pin -> through resistor (~330 ohm) -> Strip DIN pad
        Measure resistance: should read 300-370 ohms

    [ ] Common ground:
        ESP32 GND <-> Strip GND pad:    should BEEP (continuity)
        ESP32 GND <-> PSU GND (-):      should BEEP (continuity)

    [ ] Power to strip:
        PSU +5V (+) <-> Strip 5V pad:   should BEEP (continuity)
        PSU GND (-) <-> Strip GND pad:  should BEEP (continuity)

    [ ] No shorts:
        Strip 5V pad <-> Strip GND pad: should NOT beep
        PSU +5V <-> PSU GND:            should NOT beep
        ESP32 5V pin <-> ESP32 GND pin: should NOT beep

    [ ] Capacitor polarity (if installed):
        (+) leg connected to 5V rail
        (-) leg connected to GND rail

    [ ] Mic module (if installed, S3 only):
        Module VCC <-> ESP32 3V3:    should BEEP
        Module GND <-> ESP32 GND:    should BEEP
        Module OUT <-> ESP32 GPIO 1: should BEEP
        Module VCC is NOT connected to 5V (check!)

    [ ] Visual inspection:
        No exposed wire touching adjacent pins
        No solder bridges between pads
        Strip arrows point away from the DIN connection
```

### Power-On Checks

```
    1. Plug in external PSU first (if used)
    2. Plug in USB cable to ESP32
    3. Expected serial output (115200 baud):
       - "Matter LED Ecosystem v0.1.0"
       - "LED strip initialized: 60 LEDs on GPIO XX"
       - "Device not commissioned - starting BLE advertising"
    4. Expected LED behavior:
       - Strip shows blue breathing animation (commissioning mode)
       - OR strip shows last saved color (if previously commissioned)
    5. If LEDs don't light:
       - Check Section 15 (Troubleshooting)
```

---

## 15. Troubleshooting Wiring Issues

### LEDs Don't Light Up At All

| Check | How | Expected |
|-------|-----|----------|
| Is the strip getting power? | Measure voltage across strip 5V and GND pads | 4.8V - 5.2V |
| Is the data connected? | Check continuity from GPIO through resistor to DIN | ~330 ohms |
| Is ground shared? | Check continuity between ESP32 GND and strip GND | Beep |
| Is the strip the right way around? | Check arrow direction on strip | Arrow should point AWAY from DIN |
| Is the GPIO correct? | Check board_config.h for your target | GPIO 48 (S3) or GPIO 8 (C3/C6) |
| Is the first LED dead? | Cut off the first LED and connect to the second | LEDs work from #2 onward |

### Only the First LED Works (or Shows Wrong Color)

- **Likely cause:** Damaged first LED or data signal integrity issue.
- **Fix:** Try cutting off the first LED. If the rest work, the first LED's WS2812B chip is damaged.
- **Also try:** Reduce data wire length, ensure 330 ohm resistor is present, check solder joints.

### LEDs Show Random Colors / Flickering

| Cause | Fix |
|-------|-----|
| No common ground | Connect ESP32 GND to strip GND |
| Data wire too long (>30cm) | Shorten wire, add level shifter |
| Power supply noise | Add 1000uF capacitor at strip |
| Weak data signal | Add 330R resistor, or use level shifter |
| Bad solder joint | Reflow solder connections |

### LEDs at the Far End Are Dim or Orange

- **Cause:** Voltage drop over the strip's copper traces.
- **Fix:** Inject power at the far end (see Section 11).

### ESP32 Keeps Resetting / Brownout

- **Cause:** Too many LEDs drawing power from USB.
- **Fix:** Use an external 5V power supply for the strip. Do NOT power the strip from the ESP32's 5V pin.

### Music Sync Doesn't React

| Check | Fix |
|-------|-----|
| Are you using ESP32-S3? | Music sync only works on S3 (ADC on GPIO 1) |
| Is the mic module connected to 3.3V (not 5V)? | Reconnect to 3V3 pin |
| Is the gain too low? | Turn the potentiometer clockwise for more gain |
| Is the effect set to Music Sync (#9)? | Change effect to #9 |
| Is there actually sound? | Test with loud music close to the mic |

---

## 16. Common Mistakes

| # | Mistake | What Happens | How to Fix |
|---|---------|--------------|------------|
| 1 | **No common ground** between ESP32, PSU, and strip | LEDs don't work or show garbage | Connect ALL grounds together |
| 2 | **Powering 60 LEDs from USB** | ESP32 brownout, random resets | Use external 5V PSU for the strip |
| 3 | **Connecting to DOUT instead of DIN** | No LEDs light up | Flip the strip (follow the arrows) |
| 4 | **Data wire longer than 30cm** without resistor | First LED corrupted or random colors | Shorten wire + add 330R resistor |
| 5 | **Feeding 5V into ESP32 ADC GPIO** | Permanent damage to GPIO pin | Use 3.3V for mic module VCC |
| 6 | **Reversed capacitor polarity** | Capacitor overheats, leaks, or pops | (+) to 5V, (-) to GND. Always check. |
| 7 | **Forgetting the series resistor** on data | First LED eventually dies from voltage spikes | Add 330-470 ohm resistor near strip |
| 8 | **Using thin wire for power** (28 AWG+) over long runs | Voltage drop, fire risk at high current | Use 20-22 AWG for power lines |
| 9 | **Powering ESP32 and strip from same PSU without USB** | ESP32 needs 3.3V regulated, not raw 5V | Power ESP32 via USB, strip via PSU |
| 10 | **Not cutting/soldering strip at marked lines** | Damages internal traces | Only cut at the copper pad marks |

---

## 17. Bill of Materials

### 17.1 Minimum Build (Prototype / Small Strip)

| Qty | Part | Specification | Example Part | Est. Cost |
|-----|------|--------------|--------------|-----------|
| 1 | ESP32-S3 Development Board | DevKitC-1 or equivalent | Espressif ESP32-S3-DevKitC-1-N8R8 | $8-12 |
| 1 | WS2812B LED Strip | 60 LED/m, 5V, IP30 (indoor) | BTF-LIGHTING WS2812B | $5-8 |
| 1 | 5V Power Supply | 5A, regulated | Mean Well RS-25-5, or any 5V 5A | $7-12 |
| 1 | Resistor | 330 ohm, 1/4W, any tolerance | - | $0.05 |
| 1 | Capacitor | 1000uF, 25V, electrolytic | - | $0.30 |
| 3+ | Hookup Wire | 22 AWG, solid or stranded | - | $0.50 |
| 1 | USB-C Cable | Data-capable (not charge-only) | - | $3-5 |

**Estimated total: ~$25-38**

### 17.2 Full Build (S3 with Music Sync)

Add to the minimum build:

| Qty | Part | Specification | Example Part | Est. Cost |
|-----|------|--------------|--------------|-----------|
| 1 | Microphone Amplifier Module | Electret + preamp, analog out | MAX4466 breakout (Adafruit #1063) | $3-5 |
| 3 | Jumper Wires (female-female) | For mic module connections | Dupont wires | $0.50 |

**Estimated total: ~$29-44**

### 17.3 Alternative Boards

| Board | Price | Notes |
|-------|-------|-------|
| ESP32-C3-DevKitM-1 | $5-8 | Cheapest option, no music sync |
| ESP32-C6-DevKitC-1 | $7-10 | Wi-Fi 6 (802.11ax), no music sync |
| ESP32-S3-DevKitC-1 | $8-12 | Full features including music sync |
| Seeed XIAO ESP32-S3 | $6-8 | Tiny form factor, same S3 chip |

### 17.4 Where to Buy

- **Espressif Official Store** (Aliexpress/Mouser): Dev boards
- **Amazon / Aliexpress**: WS2812B strips, power supplies, misc components
- **Adafruit / SparkFun**: MAX4466 module, level shifters, quality components
- **Digikey / Mouser / LCSC**: Resistors, capacitors, connectors (for bulk/precision)
