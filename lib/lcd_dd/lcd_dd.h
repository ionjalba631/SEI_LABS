#ifndef LCD_DD_H
#define LCD_DD_H

#include <Arduino.h>

void lcd_init();
void lcd_clear();
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_print(const char *text);
void lcd_print_row(uint8_t row, const char *text);

#endif
