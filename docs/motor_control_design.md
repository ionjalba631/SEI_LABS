# DC Motor Position Control Design

## Architecture

The project uses a layered structure built around the existing drivers:

- `ddMotorL298`
  Low-level L298N driver. It owns `ENA`, `IN1`, `IN2` and generates the PWM command.
- `motor_dd`
  Small adapter that exposes forward / reverse / stop commands to the application layer.
- `potentiometer_dd`
  Reads the analog feedback signal from the potentiometer.
- `control_pid`
  PID regulator with anti-windup by conditional integration.
- `src/main.cpp`
  Main application loop. It reads the potentiometer, converts ADC to a numeric position, applies AUTO / MANUAL logic, protections, LCD updates, and Serial Plotter reporting.

## Implemented Functionalities

The current version implements the mandatory laboratory requirements:

- rotor position reading using the analog potentiometer on `A0`
- ADC to numeric position conversion in the interval `0..100`
- PID position regulator
- bidirectional motor control through the L298N driver
- PWM speed control through `ENA`
- setpoint update from the serial interface
- AUTO / MANUAL operating modes
- LCD display for setpoint, position, error, and controller output
- Arduino Serial Plotter output in the format `SetPoint:xx,Position:xx,Output:xx`

## Protection Logic

The control loop includes the following protections:

- PWM limiting to `+/-85%`
- deadband near the setpoint to reduce vibration
- motor stop when the absolute error becomes very small
- minimum / maximum position limiting
- prevention of commanding the motor outside the valid position interval

## Serial Commands

Available serial commands:

- `<0..100>`
  Set a new setpoint directly
- `SP <0..100>`
  Set a new setpoint explicitly
- `AUTO`
  Enable PID closed-loop control
- `MAN < -85..85 >`
  Enable MANUAL mode and apply a signed PWM command
- `STOP`
  Switch to MANUAL mode with zero output
- `STATUS`
  Print the current state
- `HELP`
  Print the command list

## Runtime Data

Displayed on the LCD:

- `SP` = setpoint
- `P` = current numeric position
- `E` = position error
- `O` = applied output in percent

Sent to Serial Plotter:

- `SetPoint:<value>,Position:<value>,Output:<value>`

## Testing And Results

The following checks were verified directly during development:

- successful PlatformIO build for `megaatmega2560`
- firmware size within limits
- PID control module linked correctly into the application
- serial command parser integrated with the main loop
- Serial Plotter frame format implemented exactly as required
- protection logic compiled and connected to the final motor command

Build result obtained:

- `pio run` completed successfully
- RAM usage: `860 bytes` from `8192 bytes` (`10.5%`)
- Flash usage: `10932 bytes` from `253952 bytes` (`4.3%`)

Functional checks confirmed in the implementation:

- the potentiometer is read from `A0`
- the ADC value is converted to a numeric position in the range `0..100`
- the PID output is converted to a signed PWM command
- the motor direction is selected automatically from the sign of the controller output
- AUTO mode uses PID regulation
- MANUAL mode uses the serial PWM command directly
- the motor is stopped for very small error and inside the deadband
- the command is limited to the configured PWM range
- the command is blocked at the minimum and maximum position limits

Expected behavior during Wokwi runtime testing:

- after power-up, the LCD shows the current mode, setpoint, position, error, and output
- sending `SP 70` or `70` from Serial changes the target position
- in `AUTO` mode, the motor rotates until the measured position approaches the setpoint
- near the target, the PWM output decreases and the motor stops when the error is very small
- sending `MAN 30` or `MAN -30` switches to MANUAL mode and rotates the motor in the selected direction
- Arduino Serial Plotter receives frames in the form `SetPoint:xx,Position:xx,Output:xx`

Note:

- the build and code-path verification were completed locally
- a live Wokwi execution session was not run from this terminal, so runtime observations in the simulator should still be checked visually

## Wiring

Arduino Mega 2560 pin allocation for the L298 module:

- `D6` -> `ENA` (PWM)
- `D8` -> `IN1`
- `D9` -> `IN2`
- `GND` -> `GND`
- external motor supply -> `12V` / `VIN` on L298 module
- motor terminals -> `OUT1`, `OUT2`
- `A0` <- potentiometer wiper used as the position feedback signal

The LCD and keypad wiring remain unchanged from the existing project layout.
