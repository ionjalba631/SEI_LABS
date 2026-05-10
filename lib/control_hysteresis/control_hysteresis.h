#ifndef CONTROL_HYSTERESIS_H
#define CONTROL_HYSTERESIS_H

void control_init(int hysteresis);
int control_update(int sp, int pv);

#endif
