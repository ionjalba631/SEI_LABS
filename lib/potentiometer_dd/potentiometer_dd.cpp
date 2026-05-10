#include "potentiometer_dd.h"

namespace {

uint8_t g_potentiometer_pin = A0;

}  // namespace

void potentiometer_init(const uint8_t pin) {
    g_potentiometer_pin = pin;
    pinMode(g_potentiometer_pin, INPUT);
}

uint16_t potentiometer_read() {
    uint32_t accumulator = 0;

    for (uint8_t sample = 0; sample < 5U; ++sample) {
        accumulator += analogRead(g_potentiometer_pin);
    }

    return static_cast<uint16_t>(accumulator / 5U);
}
