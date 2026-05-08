# DC Motor Position Control Design

## Architecture

The project now follows the same layered structure already present in the repository:

- `ddMotorL298`
  Direct hardware driver for the L298 bridge. It owns the output pins and PWM generation.
- `ctrlMotor`
  Application controller. It parses STDIO commands, translates them into motor actions, updates the LCD, and reports state on Serial.
- `src/main.cpp`
  Closed-loop position controller. It reads the potentiometer, compares the measured position with the setpoint,
  decides the rotation direction through a hysteresis band, and commands the L298 driver.

## Control Objective

Implemented requirement:

- rotor position control
- potentiometer used as the position sensor
- L298 used as the bidirectional actuator driver
- fixed actuator power at 50% saturation
- clockwise / counter-clockwise positioning depending on the sign of the position error

Control law:

- if `PV < SP - HYSTERESIS`, the motor is driven forward
- if `PV > SP + HYSTERESIS`, the motor is driven backward
- if `PV` is inside the hysteresis band, the motor is stopped

This gives a simple bidirectional position loop suitable for the laboratory assignment.

## User Interaction

Chosen configuration for the assignment:

- Variant `B`
- setpoint source: `serial interface`
- displayed values: LCD and `serial interface` through `printf()`
- real-time plotting: optional `Arduino Serial Plotter` frames are available in code

The operator enters a numeric setpoint in the range `0..1023` and confirms it with `Enter`.

The serial terminal reports:

- `SP` = setpoint
- `PV` = measured position from the potentiometer
- `ERR` = position error
- `OUTPUT` = signed actuator command in percent
- `STATE` = `FORWARD`, `BACKWARD`, or `STOP`

The Serial Plotter receives frames in the format:

- `SetPoint:<value> Value:<value> Output:<value>`

## Wiring

Arduino Mega 2560 pin allocation for the L298 module:

- `D6` -> `ENA` (PWM)
- `D8` -> `IN1`
- `D9` -> `IN2`
- `GND` -> `GND`
- external motor supply -> `12V` / `VIN` on L298 module
- motor terminals -> `OUT1`, `OUT2`
- `A0` <- potentiometer wiper used as the position feedback signal

The LCD and keypad pin allocations remain unchanged from the existing project.
