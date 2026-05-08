#include "control_hysteresis.h"

namespace {

int g_hysteresis = 20;

}  // namespace

void control_init(const int hysteresis) {
    g_hysteresis = (hysteresis >= 0) ? hysteresis : 0;
}

int control_update(const int sp, const int pv) {
    if (pv < (sp - g_hysteresis)) {
        return 1;
    }

    if (pv > (sp + g_hysteresis)) {
        return -1;
    }

    return 0;
}
