#ifndef DD_MOTOR_L298_H
#define DD_MOTOR_L298_H

#include <Arduino.h>

enum dd_motor_direction_t {
    DD_MOTOR_DIRECTION_STOP = 0,
    DD_MOTOR_DIRECTION_FORWARD,
    DD_MOTOR_DIRECTION_REVERSE
};

constexpr uint8_t DD_MOTOR_L298_ENA_PIN = 6;
constexpr uint8_t DD_MOTOR_L298_IN1_PIN = 8;
constexpr uint8_t DD_MOTOR_L298_IN2_PIN = 9;

void ddMotorL298Init();
void ddMotorL298Apply(dd_motor_direction_t direction, uint8_t pwm_percent);
void ddMotorL298Stop();

#endif
