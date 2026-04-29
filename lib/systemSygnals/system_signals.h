#ifndef SYSTEM_SIGNALS_H
#define SYSTEM_SIGNALS_H

#include <Arduino.h>

typedef struct {
    int x;
    int y;
    bool button_pressed;
    bool error;
} system_state_t;

void system_update_state();
system_state_t system_get_state();

#endif
