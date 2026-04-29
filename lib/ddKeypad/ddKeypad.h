#ifndef DD_KEYPAD_H
#define DD_KEYPAD_H

#include <Arduino.h>

/*
 * ddKeypad – Hardware driver for a 4x4 matrix keypad
 * Direct AVR GPIO polling, NO external library.
 *
 * Wiring (Arduino Mega 2560):
 *   Row 0 (R1) -> Pin 22  (PA0)
 *   Row 1 (R2) -> Pin 23  (PA1)  — note: Mega maps 23→PA1
 *   Row 2 (R3) -> Pin 24  (PA2)
 *   Row 3 (R4) -> Pin 26  (PA4)
 *
 *   Col 0 (C1) -> Pin 28  (PA6)
 *   Col 1 (C2) -> Pin 30  (PC7)
 *   Col 2 (C3) -> Pin 32  (PC5)
 *   Col 3 (C4) -> Pin 34  (PC3)
 *
 * Rows are driven LOW one at a time (output).
 * Columns are read with internal pull-ups enabled (input).
 * A pressed key pulls its column LOW when its row is driven LOW.
 */

#define DD_KP_NO_KEY '\0'

void ddKeypadInit(void);

/*
 * ddKeypadGetKey – non-blocking scan.
 * Returns the character of the pressed key, or DD_KP_NO_KEY if none.
 */
char ddKeypadGetKey(void);

/*
 * ddKeypadWaitKey – blocking scan.
 * Returns only when a key is pressed and then released (debounced).
 */
char ddKeypadWaitKey(void);

#endif /* DD_KEYPAD_H */