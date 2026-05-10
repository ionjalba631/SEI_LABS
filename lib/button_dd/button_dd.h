#ifndef BUTTON_DD_H
#define BUTTON_DD_H

#include <Arduino.h>

void button_init(uint8_t pin);
bool button_is_pressed();
bool button_was_pressed();

#endif
