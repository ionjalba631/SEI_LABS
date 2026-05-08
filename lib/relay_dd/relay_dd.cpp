#include "relay_dd.h"

namespace {

constexpr uint8_t RELAY_DD_PIN = 7;
bool g_relay_enabled = false;

}  // namespace

void relay_init() {
    pinMode(RELAY_DD_PIN, OUTPUT);
    relay_set(false);
}

void relay_set(const bool enabled) {
    g_relay_enabled = enabled;
    digitalWrite(RELAY_DD_PIN, enabled ? HIGH : LOW);
}

bool relay_is_on() {
    return g_relay_enabled;
}
