#ifndef CTRL_STDIO_H
#define CTRL_STDIO_H

#include <Arduino.h>
#include <stdio.h>

/*
 * ctrlStdio – Redirects standard C STDIO streams to hardware:
 *
 *   stdout  →  HD44780 LCD   (via ddLcd driver, NO external library)
 *   stdin   →  4x4 Keypad    (via ddKeypad driver, NO external library)
 *
 * After ctrlStdioInit(), the standard C functions printf() and
 * getchar() work transparently with the physical peripherals.
 *
 * Serial (UART0) is also initialised at 9600 baud for debug mirroring.
 */

#define STDIO_UART_BAUD 9600UL

void ctrlStdioInit(void);
void ctrlStdioClearLcd(void);

/* Control pentru wrap pe LCD */
void ctrlStdioSetWrap(uint8_t enable);  /* 0 = disable wrap, 1 = enable */
void ctrlStdioSetRow(uint8_t row);      /* Setează rândul curent (0 sau 1) */
void ctrlStdioSetCol(uint8_t col);      /* Setează coloana curentă */
void ctrlStdioClearRow(uint8_t row);    /* Șterge un rând specific */

#endif /* CTRL_STDIO_H */