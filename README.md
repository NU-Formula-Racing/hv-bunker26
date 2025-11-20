# HV_Bunker2026
This repo is dedicated for the bunker project in high voltage group in season 2025-2026

## PlatformIO Setup

This project is configured for PlatformIO to flash code to ESP32.

### Prerequisites
- Install [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) (VS Code extension recommended)
- Or install PlatformIO Core (CLI): `pip install platformio`

### Building and Flashing

**Using PlatformIO IDE (VS Code):**
1. Open the project in VS Code
2. Click the PlatformIO icon in the sidebar
3. Click "Build" to compile the code
4. Click "Upload" to flash to ESP32
5. Click "Monitor" to view serial output

**Using PlatformIO CLI:**
```bash
# Build the project
pio run

# Upload to ESP32 (make sure ESP32 is connected via USB)
pio run --target upload

# Monitor serial output
pio device monitor
```

### Project Structure
- `platformio.ini` - PlatformIO configuration file
- `src/main.cpp` - Main source code
- `.gitignore` - Git ignore file for PlatformIO artifacts

### Serial Monitor
The serial monitor is configured at 115200 baud. You can view printf/Serial output from the ESP32 through the USB port.
