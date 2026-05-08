#ifndef CTRL_STDIO_H
#define CTRL_STDIO_H

#include <Arduino.h>
#include <stdio.h>

/*
 * ctrlStdio – Simplified STDIO bridge using only Serial
 * 
 * Redirects stdout to Serial (UART0) for debug output.
 */

#define STDIO_UART_BAUD 9600UL

void ctrlStdioInit(void);

#endif /* CTRL_STDIO_H */