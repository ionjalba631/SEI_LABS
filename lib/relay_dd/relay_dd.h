#ifndef RELAY_DD_H
#define RELAY_DD_H

#include <Arduino.h>

void relay_init();
void relay_set(bool enabled);
bool relay_is_on();

#endif
