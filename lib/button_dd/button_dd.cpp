#include "button_dd.h"

namespace {

constexpr unsigned long BUTTON_DEBOUNCE_MS = 30UL;

uint8_t g_button_pin = 2;
bool g_last_sample = false;
bool g_stable_state = false;
bool g_press_consumed = false;
unsigned long g_last_transition_ms = 0;

bool read_raw_button() {
    return digitalRead(g_button_pin) == LOW;
}

}  // namespace

void button_init(const uint8_t pin) {
    g_button_pin = pin;
    pinMode(g_button_pin, INPUT_PULLUP);

    g_last_sample = read_raw_button();
    g_stable_state = g_last_sample;
    g_press_consumed = false;
    g_last_transition_ms = millis();
}

bool button_is_pressed() {
    const bool sample = read_raw_button();
    const unsigned long now = millis();

    if (sample != g_last_sample) {
        g_last_sample = sample;
        g_last_transition_ms = now;
    }

    if ((now - g_last_transition_ms) >= BUTTON_DEBOUNCE_MS) {
        g_stable_state = sample;
    }

    return g_stable_state;
}

bool button_was_pressed() {
    const bool pressed = button_is_pressed();

    if (!pressed) {
        g_press_consumed = false;
        return false;
    }

    if (g_press_consumed) {
        return false;
    }

    g_press_consumed = true;
    return true;
}
