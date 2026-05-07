#include "relayDriver.h"

namespace {

bool relay_state = false;

uint8_t relay_output_level(const bool enabled) {
    if (enabled) {
        return RELAY_ACTIVE_LEVEL;
    }

    return RELAY_ACTIVE_LEVEL == HIGH ? LOW : HIGH;
}

}  // namespace

void relayDriverInit() {
    pinMode(RELAY_PIN, OUTPUT);
    relayDriverOff();
}

void relayDriverSet(const bool enabled) {
    relay_state = enabled;
    digitalWrite(RELAY_PIN, relay_output_level(enabled));
}

void relayDriverOn() {
    relayDriverSet(true);
}

void relayDriverOff() {
    relayDriverSet(false);
}

void relayDriverToggle() {
    relayDriverSet(!relay_state);
}

bool relayDriverIsOn() {
    return relay_state;
}
