#include "ctrlStdio.h"
#include "ddLcd.h"
#include "ddKeypad.h"

/*
 * ctrlStdio – STDIO bridge
 *
 * Uses avr-libc's fdev_setup_stream() to bind two callbacks to a
 * FILE object that becomes stdout AND stdin.  No external library.
 *
 * LCD cursor tracking:
 *   - Characters are written left-to-right on the current row.
 *   - '\n' advances to the next row (wraps at LCD_ROWS) and clears it.
 *   - '\r' returns to column 0 on the current row.
 *   - The LCD has 2 rows x 16 columns.
 */

/* ------------------------------------------------------------------ */
/*  Cursor state                                                         */
/* ------------------------------------------------------------------ */
static uint8_t curCol = 0;
static uint8_t curRow = 0;
static uint8_t autoWrap = 1;  /* Default: wrap enabled for printf normal */

/* ------------------------------------------------------------------ */
/*  stdout callback – one character at a time to the LCD               */
/* ------------------------------------------------------------------ */
static int lcdPutchar(char c, FILE *stream) {
    (void)stream;

    /* Also mirror to Serial for debug */
    Serial.write(c);

    if (c == '\r') {
        curCol = 0;
        ddLcdSetCursor(curCol, curRow);
        return 0;
    }

    if (c == '\n') {
        /* Move to start of next row, clear it */
        curRow = (curRow + 1) % DD_LCD_ROWS;
        curCol = 0;
        ddLcdSetCursor(curCol, curRow);
        /* Erase the new row so old text doesn't bleed through */
        for (uint8_t i = 0; i < DD_LCD_COLS; i++) {
            ddLcdPutChar(' ');
        }
        ddLcdSetCursor(curCol, curRow);
        return 0;
    }

    /* Normal printable character */
    ddLcdSetCursor(curCol, curRow);
    ddLcdPutChar(c);
    curCol++;

    /* Wrap to next row only if autoWrap is enabled */
    if (autoWrap && curCol >= DD_LCD_COLS) {
        curCol = 0;
        curRow = (curRow + 1) % DD_LCD_ROWS;
        ddLcdSetCursor(curCol, curRow);
    } else if (curCol >= DD_LCD_COLS) {
        /* Keep cursor at end but don't wrap */
        curCol = DD_LCD_COLS;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  stdin callback – blocking wait for one keypad press                 */
/* ------------------------------------------------------------------ */
static int keypadGetchar(FILE *stream) {
    (void)stream;

    char key = ddKeypadWaitKey();   /* Blocks + debounces (see ddKeypad) */

    /* Mirror to Serial for debug */
    Serial.write(key);

    return (unsigned char)key;
}

/* ------------------------------------------------------------------ */
/*  Single FILE object shared by stdout and stdin                       */
/* ------------------------------------------------------------------ */
static FILE stdio_stream;

/* ------------------------------------------------------------------ */
/*  Public API                                                           */
/* ------------------------------------------------------------------ */

void ctrlStdioInit(void) {
    /* Initialise hardware drivers */
    ddLcdInit();
    ddKeypadInit();

    /* Initialise UART for debug mirroring */
    Serial.begin(STDIO_UART_BAUD);

    /* Bind callbacks and redirect streams */
    fdev_setup_stream(&stdio_stream,
                      lcdPutchar,
                      keypadGetchar,
                      _FDEV_SETUP_RW);

    stdout = &stdio_stream;
    stdin  = &stdio_stream;
}

void ctrlStdioClearLcd(void) {
    ddLcdClear();
    curCol = 0;
    curRow = 0;
}

/* ------------------------------------------------------------------ */
/*  Control functions for LCD cursor and wrap                          */
/* ------------------------------------------------------------------ */

void ctrlStdioSetWrap(uint8_t enable) {
    autoWrap = enable ? 1 : 0;
}

void ctrlStdioSetRow(uint8_t row) {
    if (row < DD_LCD_ROWS) {
        curRow = row;
    }
}

void ctrlStdioSetCol(uint8_t col) {
    if (col < DD_LCD_COLS) {
        curCol = col;
    }
}

void ctrlStdioClearRow(uint8_t row) {
    if (row < DD_LCD_ROWS) {
        uint8_t savedRow = curRow;
        uint8_t savedCol = curCol;
        
        curRow = row;
        curCol = 0;
        ddLcdSetCursor(curCol, curRow);
        
        /* Fill row with spaces */
        for (uint8_t i = 0; i < DD_LCD_COLS; i++) {
            ddLcdPutChar(' ');
        }
        
        /* Restore cursor */
        curRow = savedRow;
        curCol = savedCol;
        ddLcdSetCursor(curCol, curRow);
    }
}