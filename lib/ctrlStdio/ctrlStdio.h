#ifndef CTRL_STDIO_H
#define CTRL_STDIO_H

#include <Arduino.h>
#include <stdio.h>

#define UART_BAUD_RATE 9600

void ctrlStdioInit(unsigned long baud);

#endif