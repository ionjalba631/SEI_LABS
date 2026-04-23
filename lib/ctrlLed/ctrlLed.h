#ifndef CTRL_LED_H
#define CTRL_LED_H

#include "ddLed.h"

// Actualizat pentru Arduino Mega (Pin 13 este LED-ul on-board)
#define APP_LED_PIN 13 
#define CMD_BUFFER_SIZE 32

typedef enum {
    CMD_ON = 0,
    CMD_OFF,
    CMD_COUNT
} CtrlCmd;

void ctrlLedInit(void);
void ctrlLedUpdate(void);

#endif