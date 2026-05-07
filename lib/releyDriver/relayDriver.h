#ifndef RELAY_DRIVER_H
#define RELAY_DRIVER_H

#include <Arduino.h>

constexpr uint8_t RELAY_PIN = 7;
constexpr uint8_t RELAY_ACTIVE_LEVEL = HIGH;

void relayDriverInit();
void relayDriverSet(bool enabled);
void relayDriverOn();
void relayDriverOff();
void relayDriverToggle();
bool relayDriverIsOn();

#endif
