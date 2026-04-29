#include "system_signals.h"

#include <Arduino_FreeRTOS.h>
#include <semphr.h>

#include "joystick_driver.h"

namespace {

constexpr uint16_t ADC_MIN = 0;
constexpr uint16_t ADC_MAX = 1023;
constexpr uint16_t VREF_MV = 5000;
constexpr int16_t POSITION_MIN_PERCENT = -100;
constexpr int16_t POSITION_MAX_PERCENT = 100;
constexpr int16_t DEADZONE_PERCENT = 5;
constexpr size_t MEDIAN_WINDOW_SIZE = 3;
constexpr size_t WEIGHTED_WINDOW_SIZE = 5;
constexpr uint8_t WEIGHTED_FILTER_WEIGHTS[WEIGHTED_WINDOW_SIZE] = {1, 2, 3, 2, 1};

struct axis_filter_state_t {
    uint16_t raw_history[MEDIAN_WINDOW_SIZE];
    uint16_t weighted_history[WEIGHTED_WINDOW_SIZE];
    size_t raw_index;
    size_t weighted_index;
    bool initialized;
};

system_state_t g_system_state = {};
SemaphoreHandle_t g_state_mutex = nullptr;
axis_filter_state_t g_x_filter = {};
axis_filter_state_t g_y_filter = {};

void ensure_state_mutex() {
    if (g_state_mutex == nullptr) {
        g_state_mutex = xSemaphoreCreateMutex();
    }
}

uint16_t saturate_u16(const long value, const uint16_t min_value, const uint16_t max_value, bool &saturated) {
    if (value < min_value) {
        saturated = true;
        return min_value;
    }

    if (value > max_value) {
        saturated = true;
        return max_value;
    }

    return static_cast<uint16_t>(value);
}

int16_t saturate_i16(const long value, const int16_t min_value, const int16_t max_value, bool &saturated) {
    if (value < min_value) {
        saturated = true;
        return min_value;
    }

    if (value > max_value) {
        saturated = true;
        return max_value;
    }

    return static_cast<int16_t>(value);
}

void seed_filter(axis_filter_state_t &filter_state, const uint16_t sample) {
    for (size_t index = 0; index < MEDIAN_WINDOW_SIZE; ++index) {
        filter_state.raw_history[index] = sample;
    }

    for (size_t index = 0; index < WEIGHTED_WINDOW_SIZE; ++index) {
        filter_state.weighted_history[index] = sample;
    }

    filter_state.raw_index = 0;
    filter_state.weighted_index = 0;
    filter_state.initialized = true;
}

uint16_t median_of_three(const uint16_t a, const uint16_t b, const uint16_t c) {
    if ((a >= b && a <= c) || (a >= c && a <= b)) {
        return a;
    }

    if ((b >= a && b <= c) || (b >= c && b <= a)) {
        return b;
    }

    return c;
}

uint16_t apply_salt_pepper_filter(axis_filter_state_t &filter_state, const uint16_t sample) {
    if (!filter_state.initialized) {
        seed_filter(filter_state, sample);
    }

    filter_state.raw_history[filter_state.raw_index] = sample;
    filter_state.raw_index = (filter_state.raw_index + 1U) % MEDIAN_WINDOW_SIZE;

    return median_of_three(
        filter_state.raw_history[0],
        filter_state.raw_history[1],
        filter_state.raw_history[2]
    );
}

uint16_t apply_weighted_average_filter(axis_filter_state_t &filter_state, const uint16_t sample) {
    filter_state.weighted_history[filter_state.weighted_index] = sample;
    filter_state.weighted_index = (filter_state.weighted_index + 1U) % WEIGHTED_WINDOW_SIZE;

    uint32_t weighted_sum = 0;
    uint16_t total_weight = 0;

    for (size_t offset = 0; offset < WEIGHTED_WINDOW_SIZE; ++offset) {
        const size_t history_index =
            (filter_state.weighted_index + offset) % WEIGHTED_WINDOW_SIZE;
        const uint8_t weight = WEIGHTED_FILTER_WEIGHTS[offset];

        weighted_sum += static_cast<uint32_t>(filter_state.weighted_history[history_index]) * weight;
        total_weight += weight;
    }

    return static_cast<uint16_t>(weighted_sum / total_weight);
}

int16_t adc_to_position_percent(const uint16_t adc_value, bool &saturated) {
    const long scaled = map(adc_value, ADC_MIN, ADC_MAX, POSITION_MIN_PERCENT, POSITION_MAX_PERCENT);
    int16_t percent = saturate_i16(scaled, POSITION_MIN_PERCENT, POSITION_MAX_PERCENT, saturated);

    if (abs(percent) <= DEADZONE_PERCENT) {
        percent = 0;
    }

    return percent;
}

axis_signal_t build_axis_state(axis_filter_state_t &filter_state, const int raw_sample) {
    axis_signal_t axis_state = {};
    bool saturated = false;

    axis_state.raw_adc = saturate_u16(raw_sample, ADC_MIN, ADC_MAX, saturated);
    axis_state.salt_pepper_filtered_adc = apply_salt_pepper_filter(filter_state, axis_state.raw_adc);
    axis_state.weighted_filtered_adc =
        apply_weighted_average_filter(filter_state, axis_state.salt_pepper_filtered_adc);
    axis_state.voltage_mv = static_cast<uint16_t>(
        (static_cast<uint32_t>(axis_state.weighted_filtered_adc) * VREF_MV) / ADC_MAX
    );
    axis_state.position_percent = adc_to_position_percent(axis_state.weighted_filtered_adc, saturated);
    axis_state.saturated = saturated;

    return axis_state;
}

}  // namespace

void system_init() {
    ensure_state_mutex();

    const uint16_t initial_x = static_cast<uint16_t>(joystick_read_x());
    const uint16_t initial_y = static_cast<uint16_t>(joystick_read_y());

    seed_filter(g_x_filter, initial_x);
    seed_filter(g_y_filter, initial_y);
}

void system_update_state() {
    ensure_state_mutex();

    system_state_t next_state = {};
    next_state.x = build_axis_state(g_x_filter, joystick_read_x());
    next_state.y = build_axis_state(g_y_filter, joystick_read_y());
    next_state.button_pressed = joystick_read_button();
    next_state.error = next_state.x.saturated || next_state.y.saturated;

    if (g_state_mutex != nullptr &&
        xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        g_system_state = next_state;
        xSemaphoreGive(g_state_mutex);
    }
}

system_state_t system_get_state() {
    ensure_state_mutex();

    system_state_t current_state = {};
    current_state.error = true;

    if (g_state_mutex != nullptr &&
        xSemaphoreTake(g_state_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        current_state = g_system_state;
        xSemaphoreGive(g_state_mutex);
    }

    return current_state;
}
