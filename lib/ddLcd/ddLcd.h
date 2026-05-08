#ifndef DD_LCD_H
#define DD_LCD_H

#include <Arduino.h>

/*
 * ddLcd – Hardware driver for HD44780-compatible 16x2 LCD
 * 4-bit mode, direct AVR port manipulation, NO external library.
 *
 * Wiring (Arduino Mega 2560):
 *   RS  -> Pin 25   (PA3)
 *   E   -> Pin 27   (PA5)
 *   D4  -> Pin 29   (PA7)
 *   D5  -> Pin 31   (PC6)
 *   D6  -> Pin 33   (PC4)
 *   D7  -> Pin 35   (PC2)
 *   RW  -> GND (always write)
 *   VSS -> GND
 *   VDD -> 5V
 *   VO  -> 10k pot middle pin (contrast)
 *   A   -> 5V via 220Ω (backlight)
 *   K   -> GND
 */

#define DD_LCD_COLS 16
#define DD_LCD_ROWS  2

void ddLcdInit(void);
void ddLcdClear(void);
void ddLcdSetCursor(uint8_t col, uint8_t row);
void ddLcdPutChar(char c);
void ddLcdPrint(const char *str);

#endif /* DD_LCD_H */