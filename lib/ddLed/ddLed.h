#ifndef DD_LED_H
#define DD_LED_H

#include <Arduino.h>

typedef enum {
    DD_LED_OFF = LOW,
    DD_LED_ON = HIGH
} DdLedState;

typedef struct DdLed {
    uint8_t pinNumber;
    DdLedState state;
    void (*setHwState)(uint8_t pin, uint8_t val);
} DdLed;

void ddLedInit(DdLed *led, uint8_t pin);
void ddLedOn(DdLed *led);
void ddLedOff(DdLed *led);

#endif