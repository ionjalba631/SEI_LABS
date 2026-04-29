#ifndef SYSTEM_SIGNALS_H
#define SYSTEM_SIGNALS_H

#include <Arduino.h>

typedef struct {
    uint16_t raw_adc;
    uint16_t salt_pepper_filtered_adc;
    uint16_t weighted_filtered_adc;
    uint16_t voltage_mv;
    int16_t position_percent;
    bool saturated;
} axis_signal_t;

typedef struct {
    axis_signal_t x;
    axis_signal_t y;
    bool button_pressed;
    bool error;
} system_state_t;

void system_init();
void system_update_state();
system_state_t system_get_state();

#endif
