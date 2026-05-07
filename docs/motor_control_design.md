# DC Motor Control Design

## Architecture

The project now follows the same layered structure already present in the repository:

- `ddMotorL298`
  Direct hardware driver for the L298 bridge. It owns the output pins and PWM generation.
- `ctrlMotor`
  Application controller. It parses STDIO commands, translates them into motor actions, updates the LCD, and reports state on Serial.
- `src/main.cpp`
  Minimal bootstrap: initializes STDIO redirection to Serial and calls the controller update loop.

## Implemented Commands

The serial terminal accepts:

- `motor set -100..100`
- `motor stop`
- `motor max`
- `motor inc`
- `motor dec`
- `motor status`
- `help`

Keypad shortcuts:

- `A` increase power by 10%
- `B` decrease power toward 0 by 10%
- `C` set max power in current direction
- `D` stop motor
- `#` report status
- `*` reverse direction
- `0..9` preset power 0..90% in the remembered direction

## Wiring

Arduino Mega 2560 pin allocation for the L298 module:

- `D6` -> `ENA` (PWM)
- `D8` -> `IN1`
- `D9` -> `IN2`
- `GND` -> `GND`
- external motor supply -> `12V` / `VIN` on L298 module
- motor terminals -> `OUT1`, `OUT2`

The LCD and keypad pin allocations remain unchanged from the existing project.
