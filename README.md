# The Raw Alarm Clock

A custom PCB alarm clock built around a Seeed XIAO-ESP32-C3, with a 4-button interface, buzzer alarm, ST7789 display, and a 3D-printed, F1-car-inspired case.

## Overview

- **MCU:** Seeed XIAO-ESP32-C3
- **Display:** ST7789 TFT, 284x76 (portrait panel, driven in landscape)
- **Input:** 4x push buttons (mode / up / down / alarm toggle)
- **Output:** Piezo buzzer for alarm
- **Case:** Custom 3D-printed enclosure styled after an F1 car (nose cone, side pod vents, rear wing, corner "wheels")

## Hardware

| Component | Connection |
|---|---|
| Display DC | D4 |
| Display CS | D5 |
| Display BL | D6 |
| Buzzer | D7 |
| Display RST | D8 |
| Display SCL (SPI clock) | D9 |
| Display SDA (SPI data) | D10 |
| Button 1 (Mode) | D0 |
| Button 2 (Up) | D1 |
| Button 3 (Down) | D2 |
| Button 4 (Alarm toggle) | D3 |

PCB designed in KiCad. 4x mounting holes at the corners for case assembly.

## Case

Designed in Onshape, modeled off the real PCB dimensions (99mm x 51.5mm board). Two-piece design:

- `bottom_shell.stl` — walls, floor, PCB standoff posts, side pod vents, front splitter, wheel accents
- `top_lid.stl` — display cutout (halo-style frame) and button holes
- `rear_wing.stl` — separate decorative piece, print and attach after
- The case may look bit funny as due to me not having accurate measurements. After PCB arrives, I will edit it accordingly.

**Note:** button and display cutout dimensions in the lid are estimates — it will be verify against the actual parts/pcb before final printing using my A1 Mini.

## Firmware

Built in Arduino IDE. Requires:
- Adafruit GFX library
- Adafruit ST7789 library

Flash `RawAlarmClock.ino` to the XIAO-ESP32-C3.

**Controls:**
- **Mode button** — cycles: Clock display -> Set Hour -> Set Minute -> Set Alarm Hour -> Set Alarm Minute
- **Up / Down** — adjust value in a "set" mode
- **Alarm button** — toggle alarm on/off
- When the alarm rings, press Mode or Alarm to dismiss

Time is tracked in software (no RTC chip on this board), so it resets to 00:00 on power loss.

## Build Instructions

1. Fabricate the PCB from the KiCad files (see `/PCB`)/ Gerber files in Production Folder
2. Solder components: XIAO-ESP32-C3, ST7789 display, buzzer, 4x push buttons
3. 3D print `bottom_shell.stl`, `top_lid.stl`, and `rear_wing.stl`
4. Flash `RawAlarmClock.ino` via Arduino IDE
5. Assemble PCB into the case, screw down using the corner posts, attach the lid and rear wing

## Media
<img width="552" height="285" alt="Screenshot 2026-09-20 at 12 42 52 PM" src="https://github.com/user-attachments/assets/054860c1-3a3d-421e-9c54-d16c7d509fa4" />
<img width="848" height="515" alt="Screenshot 2026-09-20 at 12 43 01 PM" src="https://github.com/user-attachments/assets/c6d45b04-5bc5-4d73-aabb-346d895d7f90" />
Case is in 3 parts hence photo of case will be clear after assembled fully


## Credits

Built by Aditya as part of the BLARE.
