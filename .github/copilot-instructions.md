# SEI LABS - Copilot Instructions

## Project Overview
Arduino embedded systems project targeting **ATmega2560** (Arduino Mega) using **PlatformIO**. Educational codebase for microcontroller programming with Wokwi circuit simulation.

## Architecture & Key Components

### Build System
- **Framework**: PlatformIO with Arduino framework
- **Build Target**: `[env:megaatmega2560]` in `platformio.ini`
- **Output**: Firmware builds to `.pio/build/megaatmega2560/firmware.{hex,elf}`
- **Toolchain**: AVR GCC (auto-managed by PlatformIO)

### Testing & Simulation
- **Simulator**: Wokwi (browser-based Arduino simulator)
- **Circuit Definition**: `test/diagram.json` - contains circuit topology (Arduino Mega, LEDs, buttons, resistors)
- **Wokwi Config**: `test/wokwi.toml` - links firmware build output to simulation
- **Typical Flow**: Build → Wokwi loads firmware.hex → Run simulation

### Code Structure
- **main.cpp**: Single sketch file with `setup()` and `loop()` pattern (Arduino standard)
- **I/O Pattern**: Uses `pinMode()`, `digitalRead()`, `digitalWrite()` for GPIO
- **Example**: Pin 2 = button input (INPUT_PULLUP), Pin 13 = LED output
- **Comments**: Project uses Romanian language in code comments

## Developer Workflows

### Build & Simulation (Primary)
```bash
# Build firmware
pio run -e megaatmega2560

# Run simulation (via VS Code or directly)
# Wokwi will automatically load built firmware from .pio/build/megaatmega2560/firmware.hex
```

### Hardware Deployment (When Needed)
Currently configured for simulation-only. To deploy to physical Arduino Mega:
```bash
# Configure upload port and speed in platformio.ini:
# [env:megaatmega2560]
# upload_port = COM3  (or /dev/ttyUSB0 on Linux)
# upload_speed = 115200

# Build and upload
pio run -e megaatmega2560 -t upload

# Monitor serial output (9600 baud default)
pio device monitor -e megaatmega2560
```

### Serial Communication
Add to `main.cpp` for hardware debugging output:
```cpp
void setup() {
    Serial.begin(9600);  // ATmega2560 standard baud rate
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(ledPin, OUTPUT);
}

void loop() {
    Serial.println("Button state: " + String(digitalRead(buttonPin)));
    // ... rest of loop
}
```
Wokwi simulator does not capture Serial output—only use for hardware testing.

### Debugging
- **VS Code Configurations**: Three debug modes in `.vscode/launch.json`:
  - "PIO Debug" - Build + Debug
  - "PIO Debug (skip Pre-Debug)" - Debug existing build
  - "PIO Debug (without uploading)" - Manual load mode
- **Pre-Debug Task**: Automatically compiles before debugging session

## Project-Specific Conventions

### Code Style
- Arduino C++ with `#include <Arduino.h>` at file start
- Pin assignments as module-level constants: `const int buttonPin = 2;`
- Comments may be in Romanian (educational project context)

### GPIO & Hardware Constraints
- **ATmega2560 specifics**: 54 digital I/O pins, limited RAM (~8KB)
- **Pullup Resistors**: Used via INPUT_PULLUP for button inputs (see line 7 in main.cpp)
- **Circuit Design**: 220Ω current-limiting resistor for LED (test/diagram.json)

### Testing Integration
- **Wokwi Workflow**: Update circuit in `diagram.json`, firmware loads automatically via `wokwi.toml`
- **No Physical Hardware Required**: Simulate entire circuit before uploading

## Integration Points
- **PlatformIO CLI**: Underlies all build/debug operations
- **Wokwi Simulator**: Direct integration via firmware path in wokwi.toml
- **VS Code PlatformIO Extension**: Provides task runners and debug configurations
- **Firmware Artifacts**: 
  - `.pio/build/megaatmega2560/firmware.hex` - Used by Wokwi simulator and hardware upload
  - `.pio/build/megaatmega2560/firmware.elf` - Debug symbols for VS Code debugger
- **Upload Configuration**: Stored in `platformio.ini` under `[env:megaatmega2560]` (currently minimal—add `upload_port` and `upload_speed` for hardware)

## Reference Files
- [platformio.ini](../platformio.ini) - Board & build configuration
- [src/main.cpp](../src/main.cpp) - Arduino sketch example
- [test/diagram.json](../test/diagram.json) - Circuit schematic definition
- [.vscode/launch.json](../.vscode/launch.json) - Debug configurations
