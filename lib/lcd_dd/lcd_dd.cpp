#include "lcd_dd.h"

#include "ddLcd.h"

void lcd_init() {
    ddLcdInit();
}

void lcd_clear() {
    ddLcdClear();
}

void lcd_set_cursor(const uint8_t col, const uint8_t row) {
    ddLcdSetCursor(col, row);
}

void lcd_print(const char *text) {
    ddLcdPrint(text);
}

void lcd_print_row(const uint8_t row, const char *text) {
    ddLcdSetCursor(0, row);

    uint8_t column = 0;
    while (column < DD_LCD_COLS && text[column] != '\0') {
        ddLcdPutChar(text[column]);
        ++column;
    }

    while (column < DD_LCD_COLS) {
        ddLcdPutChar(' ');
        ++column;
    }
}
