#ifndef LED_FSM_H
#define LED_FSM_H

#include <Arduino.h>

typedef enum {
    LED_FSM_STATE_OFF = 0,
    LED_FSM_STATE_ON = 1
} LedFsmState;

typedef struct {
    LedFsmState previous_state;
    LedFsmState current_state;
    bool changed;
} LedFsmTransition;

void led_fsm_init();
LedFsmState led_fsm_get_state();
LedFsmTransition led_fsm_update(bool button_pressed_event);

#endif
