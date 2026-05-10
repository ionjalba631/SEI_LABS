#include "motor_dd.h"

#include "ddMotorL298.h"

void motor_init() {
    ddMotorL298Init();
}

void motor_apply(const motor_dd_state_t state, const uint8_t pwm_percent) {
    switch (state) {
        case MOTOR_DD_FORWARD:
            ddMotorL298Apply(DD_MOTOR_DIRECTION_FORWARD, pwm_percent);
            break;

        case MOTOR_DD_BACKWARD:
            ddMotorL298Apply(DD_MOTOR_DIRECTION_REVERSE, pwm_percent);
            break;

        case MOTOR_DD_STOP:
        default:
            ddMotorL298Stop();
            break;
    }
}

void motor_stop() {
    ddMotorL298Stop();
}
