#include "led_fsm.h"

namespace {

LedFsmState g_current_state = LED_FSM_STATE_OFF;

LedFsmState next_state_for_event(const LedFsmState current_state) {
    if (current_state == LED_FSM_STATE_OFF) {
        return LED_FSM_STATE_ON;
    }

    return LED_FSM_STATE_OFF;
}

}  // namespace

void led_fsm_init() {
    g_current_state = LED_FSM_STATE_OFF;
}

LedFsmState led_fsm_get_state() {
    return g_current_state;
}

LedFsmTransition led_fsm_update(const bool button_pressed_event) {
    LedFsmTransition transition = {
        g_current_state,
        g_current_state,
        false
    };

    if (!button_pressed_event) {
        return transition;
    }

    transition.current_state = next_state_for_event(g_current_state);
    transition.changed = transition.current_state != transition.previous_state;
    g_current_state = transition.current_state;

    return transition;
}
