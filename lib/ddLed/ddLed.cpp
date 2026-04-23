#include "ddLed.h"

// Functie statica pentru control hardware (lambda nu e suportata pe AVR)
static void ddLedSetPin(uint8_t pin, uint8_t val) {
    digitalWrite(pin, val);
}

void ddLedInit(DdLed *led, uint8_t pin) {
    if (led == NULL) return;
    led->pinNumber = pin;
    led->state = DD_LED_OFF;
    led->setHwState = ddLedSetPin;

    pinMode(led->pinNumber, OUTPUT);
    led->setHwState(led->pinNumber, led->state);
}

void ddLedOn(DdLed *led) {
    if (led == NULL) return;
    led->state = DD_LED_ON;
    led->setHwState(led->pinNumber, led->state);
}

void ddLedOff(DdLed *led) {
    if (led == NULL) return;
    led->state = DD_LED_OFF;
    led->setHwState(led->pinNumber, led->state);
}