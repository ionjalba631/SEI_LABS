# Smart Traffic Light Design

## Architecture

The application is split into small modules so the finite-state control logic stays independent from hardware details:

- `src/main.cpp`
  Creates the FreeRTOS tasks, initializes the LEDs, LCD, Serial, and the North request button.
- `lib/traffic_fsm`
  Encapsulates the traffic-light finite-state machine and exposes snapshots of the current intersection state.
- `lib/button_dd`, `lib/ddLed`, `lib/lcd_dd`
  Reused drivers from the existing project for digital input, LED output, and LCD updates.

## FreeRTOS Tasks

- `sensor_task`
  Polls the North request button with debounce support and signals a binary semaphore when a valid request is detected.
- `controller_task`
  Owns the finite-state machine, consumes request events, applies the active light pattern, and updates the shared snapshot.
- `display_task`
  Prints the current state as JSON on Serial and mirrors a compact summary on the LCD.

## FSM Behavior

Implemented sequence:

1. `EAST_GREEN`
2. `EAST_YELLOW`
3. `ALL_RED_TO_NORTH`
4. `NORTH_GREEN`
5. `NORTH_YELLOW`
6. `ALL_RED_TO_EAST`
7. return to `EAST_GREEN`

Rules:

- East has priority and remains green if there is no request from North.
- A North request is latched until the controller actually grants `NORTH_GREEN`.
- Every direction respects the legal order `GREEN -> YELLOW -> RED`.
- An intermediate all-red phase is inserted between directions for safer switching.

## Timing

- East minimum green: `5000 ms`
- North green: `4000 ms`
- Yellow: `2000 ms`
- All-red: `1000 ms`

These values are configured in `lib/traffic_fsm/traffic_fsm.cpp` and can be tuned easily.

## Output Format

The Serial monitor emits JSON snapshots compatible with `docs/traffic_state.schema.json`, for example:

```json
{
  "phase": "EAST_GREEN",
  "east": "GREEN",
  "north": "RED",
  "northRequestPending": false,
  "phaseElapsedMs": 2300,
  "phaseDurationMs": 5000
}
```
