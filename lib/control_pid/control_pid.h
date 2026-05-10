#ifndef CONTROL_PID_H
#define CONTROL_PID_H

struct control_pid_config_t {
    float kp;
    float ki;
    float kd;
    float output_min;
    float output_max;
};

void control_pid_init(const control_pid_config_t &config);
void control_pid_reset(float measurement);
float control_pid_update(float setpoint, float measurement, float dt_seconds);

#endif
