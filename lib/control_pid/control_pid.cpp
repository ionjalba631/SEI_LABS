#include "control_pid.h"

namespace {

control_pid_config_t g_config = {0.0f, 0.0f, 0.0f, -100.0f, 100.0f};
float g_integral = 0.0f;
float g_previous_measurement = 0.0f;
bool g_has_previous_measurement = false;

float clamp_float(const float value, const float minimum, const float maximum) {
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

}  // namespace

void control_pid_init(const control_pid_config_t &config) {
    g_config = config;

    if (g_config.output_min > g_config.output_max) {
        const float temporary = g_config.output_min;
        g_config.output_min = g_config.output_max;
        g_config.output_max = temporary;
    }

    g_integral = 0.0f;
    g_previous_measurement = 0.0f;
    g_has_previous_measurement = false;
}

void control_pid_reset(const float measurement) {
    g_integral = 0.0f;
    g_previous_measurement = measurement;
    g_has_previous_measurement = true;
}

float control_pid_update(const float setpoint, const float measurement, const float dt_seconds) {
    if (dt_seconds <= 0.0f) {
        return 0.0f;
    }

    const float error = setpoint - measurement;
    const float derivative =
        g_has_previous_measurement ? (-(measurement - g_previous_measurement) / dt_seconds) : 0.0f;

    const float candidate_integral = g_integral + (error * dt_seconds);
    const float proportional_term = g_config.kp * error;
    const float derivative_term = g_config.kd * derivative;
    const float candidate_output = proportional_term + (g_config.ki * candidate_integral) + derivative_term;

    if (!((candidate_output > g_config.output_max && error > 0.0f) ||
          (candidate_output < g_config.output_min && error < 0.0f))) {
        g_integral = candidate_integral;
    }

    g_previous_measurement = measurement;
    g_has_previous_measurement = true;

    const float output = proportional_term + (g_config.ki * g_integral) + derivative_term;
    return clamp_float(output, g_config.output_min, g_config.output_max);
}
