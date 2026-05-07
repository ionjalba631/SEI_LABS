#include "ddMotorL298.h"

namespace {

uint8_t clamp_pwm_percent(const uint8_t pwm_percent) {
    if (pwm_percent > 100U) {
        return 100U;
    }

    return pwm_percent;
}

uint8_t percent_to_pwm(const uint8_t pwm_percent) {
    const uint8_t limited_percent = clamp_pwm_percent(pwm_percent);
    return static_cast<uint8_t>((static_cast<uint16_t>(limited_percent) * 255U) / 100U);
}

void set_direction_pins(const dd_motor_direction_t direction) {
    switch (direction) {
        case DD_MOTOR_DIRECTION_FORWARD:
            digitalWrite(DD_MOTOR_L298_IN1_PIN, HIGH);
            digitalWrite(DD_MOTOR_L298_IN2_PIN, LOW);
            break;

        case DD_MOTOR_DIRECTION_REVERSE:
            digitalWrite(DD_MOTOR_L298_IN1_PIN, LOW);
            digitalWrite(DD_MOTOR_L298_IN2_PIN, HIGH);
            break;

        case DD_MOTOR_DIRECTION_STOP:
        default:
            digitalWrite(DD_MOTOR_L298_IN1_PIN, LOW);
            digitalWrite(DD_MOTOR_L298_IN2_PIN, LOW);
            break;
    }
}

}  // namespace

void ddMotorL298Init() {
    pinMode(DD_MOTOR_L298_ENA_PIN, OUTPUT);
    pinMode(DD_MOTOR_L298_IN1_PIN, OUTPUT);
    pinMode(DD_MOTOR_L298_IN2_PIN, OUTPUT);

    ddMotorL298Stop();
}

void ddMotorL298Apply(const dd_motor_direction_t direction, const uint8_t pwm_percent) {
    if (direction == DD_MOTOR_DIRECTION_STOP || pwm_percent == 0U) {
        ddMotorL298Stop();
        return;
    }

    set_direction_pins(direction);
    analogWrite(DD_MOTOR_L298_ENA_PIN, percent_to_pwm(pwm_percent));
}

void ddMotorL298Stop() {
    set_direction_pins(DD_MOTOR_DIRECTION_STOP);
    analogWrite(DD_MOTOR_L298_ENA_PIN, 0);
}
