#include "led_dd.h"

#include "ddLed.h"

namespace {

DdLed g_led;
bool g_initialized = false;

}  // namespace

void led_init(const uint8_t pin) {
    ddLedInit(&g_led, pin);
    g_initialized = true;
}

void led_set(const bool enabled) {
    if (!g_initialized) {
        return;
    }

    if (enabled) {
        ddLedOn(&g_led);
        return;
    }

    ddLedOff(&g_led);
}
