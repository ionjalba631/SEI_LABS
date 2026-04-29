#include "system_signals.h"

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

#include "joystick_driver.h"

namespace {

system_state_t g_system_state = {0, 0, false, false};
SemaphoreHandle_t g_state_mutex = nullptr;

void ensure_state_mutex() {
    if (g_state_mutex == nullptr) {
        g_state_mutex = xSemaphoreCreateMutex();
    }
}

bool adc_value_invalid(const int value) {
    return (value < 0) || (value > 1023);
}

}  // namespace

void system_update_state() {
    ensure_state_mutex();

    const int raw_x = joystick_read_x();
    const int raw_y = joystick_read_y();

    system_state_t next_state;
    next_state.x = joystick_get_x_percent();
    next_state.y = joystick_get_y_percent();
    next_state.button_pressed = joystick_read_button();
    next_state.error = adc_value_invalid(raw_x) || adc_value_invalid(raw_y);

    if (g_state_mutex != nullptr &&
        xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        g_system_state = next_state;
        xSemaphoreGive(g_state_mutex);
    }
}

system_state_t system_get_state() {
    ensure_state_mutex();

    system_state_t current_state = {0, 0, false, true};

    if (g_state_mutex != nullptr &&
        xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        current_state = g_system_state;
        xSemaphoreGive(g_state_mutex);
    }

    return current_state;
}
