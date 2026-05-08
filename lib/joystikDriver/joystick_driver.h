#ifndef JOYSTICK_DRIVER_H
#define JOYSTICK_DRIVER_H

#include <Arduino.h>

void joystick_init();
int joystick_read_x();
int joystick_read_y();
bool joystick_read_button();
int joystick_get_x_percent();
int joystick_get_y_percent();

#endif
