#ifndef MOTOR_DD_H
#define MOTOR_DD_H

#include <Arduino.h>

typedef enum {
    MOTOR_DD_BACKWARD = -1,
    MOTOR_DD_STOP = 0,
    MOTOR_DD_FORWARD = 1
} motor_dd_state_t;

void motor_init();
void motor_apply(motor_dd_state_t state, uint8_t pwm_percent);
void motor_stop();

#endif
