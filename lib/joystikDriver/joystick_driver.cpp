#include "joystick_driver.h"

namespace {

constexpr uint8_t JOYSTICK_PIN_X = A0;
constexpr uint8_t JOYSTICK_PIN_Y = A1;
constexpr uint8_t JOYSTICK_PIN_SW = 2;

constexpr int ADC_MIN = 0;
constexpr int ADC_MAX = 1023;
constexpr int DEADZONE_PERCENT = 5;
constexpr unsigned long DEBOUNCE_MS = 30;

int scale_adc_to_percent(const int value) {
    const long scaled = map(value, ADC_MIN, ADC_MAX, -100, 100);
    int percent = static_cast<int>(scaled);

    if (abs(percent) <= DEADZONE_PERCENT) {
        percent = 0;
    }

    return constrain(percent, -100, 100);
}

bool read_raw_button_pressed() {
    return digitalRead(JOYSTICK_PIN_SW) == LOW;
}

}  // namespace

void joystick_init() {
    pinMode(JOYSTICK_PIN_SW, INPUT_PULLUP);
}

int joystick_read_x() {
    return analogRead(JOYSTICK_PIN_X);
}

int joystick_read_y() {
    return analogRead(JOYSTICK_PIN_Y);
}

bool joystick_read_button() {
    static bool stable_state = false;
    static bool last_sample = false;
    static unsigned long last_transition_ms = 0;

    const bool sample = read_raw_button_pressed();
    const unsigned long now = millis();

    if (sample != last_sample) {
        last_sample = sample;
        last_transition_ms = now;
    }

    if ((now - last_transition_ms) >= DEBOUNCE_MS) {
        stable_state = sample;
    }

    return stable_state;
}

int joystick_get_x_percent() {
    return scale_adc_to_percent(joystick_read_x());
}

int joystick_get_y_percent() {
    return scale_adc_to_percent(joystick_read_y());
}
