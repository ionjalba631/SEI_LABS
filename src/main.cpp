#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>

#include "control_hysteresis.h"
#include "lcd_dd.h"
#include "motor_dd.h"
#include "potentiometer_dd.h"

namespace {

constexpr uint8_t POTENTIOMETER_PIN = A0;
constexpr uint8_t MOTOR_PWM_PERCENT = 50;
constexpr int DEFAULT_SET_POINT = 512;
constexpr int HYSTERESIS_BAND = 50;
constexpr unsigned long CONTROL_PERIOD_MS = 20UL;
constexpr unsigned long LCD_PERIOD_MS = 100UL;
constexpr unsigned long SERIAL_PERIOD_MS = 250UL;
constexpr unsigned long PLOTTER_PERIOD_MS = 100UL;
constexpr size_t INPUT_BUFFER_SIZE = 8U;
constexpr bool PLOTTER_OUTPUT_ENABLED = false;

char g_serial_buffer[INPUT_BUFFER_SIZE] = "";
uint8_t g_serial_length = 0;
int g_set_point = DEFAULT_SET_POINT;
uint16_t g_process_value = 0;
int g_control_state = 0;
unsigned long g_last_control_ms = 0;
unsigned long g_last_lcd_ms = 0;
unsigned long g_last_serial_ms = 0;
unsigned long g_last_plotter_ms = 0;

#ifdef ARDUINO_ARCH_AVR
int serial_putchar(char character, FILE *stream) {
    (void)stream;

    if (character == '\n') {
        Serial.write('\r');
    }

    Serial.write(character);
    return 0;
}

FILE serial_stdout;
#endif

const char* state_to_text(const int state) {
    switch (state) {
        case 1:
            return "FORWARD";
        case -1:
            return "BACKWARD";
        case 0:
        default:
            return "STOP";
    }
}

void apply_outputs(const int control_state) {
    if (control_state > 0) {
        motor_apply(MOTOR_DD_FORWARD, MOTOR_PWM_PERCENT);
        return;
    }

    if (control_state < 0) {
        motor_apply(MOTOR_DD_BACKWARD, MOTOR_PWM_PERCENT);
        return;
    }

    motor_stop();
}

int control_output_percent() {
    return g_control_state * MOTOR_PWM_PERCENT;
}

void update_lcd() {
    char row0[17];
    char row1[17];

    snprintf(row0, sizeof(row0), "SP:%4d PV:%4u", g_set_point, g_process_value);
    snprintf(row1, sizeof(row1), "OUT:%4d %-5s", control_output_percent(), state_to_text(g_control_state));

    lcd_print_row(0, row0);
    lcd_print_row(1, row1);
}

void report_state() {
    printf(
        "\rSP=%d PV=%u ERR=%d OUTPUT=%d STATE=%s   ",
        g_set_point,
        g_process_value,
        g_set_point - static_cast<int>(g_process_value),
        control_output_percent(),
        state_to_text(g_control_state)
    );
}

void report_plotter_frame() {
    printf(
        "SetPoint:%d Value:%u Output:%d\n",
        g_set_point,
        g_process_value,
        control_output_percent()
    );
}

void commit_set_point() {
    g_serial_buffer[g_serial_length] = '\0';
    const long candidate = strtol(g_serial_buffer, nullptr, 10);

    if (candidate < 0L || candidate > 1023L) {
        printf("Setpoint invalid. Interval: 0..1023\n");
        g_serial_length = 0;
        return;
    }

    g_set_point = static_cast<int>(candidate);
    g_serial_length = 0;

    printf("Setpoint nou: %d\n", g_set_point);
}

void process_serial() {
    while (Serial.available() > 0) {
        const char received = static_cast<char>(Serial.read());

        if (received == '\r') {
            continue;
        }

        if (received == '\n') {
            if (g_serial_length > 0U) {
                commit_set_point();
            }
            continue;
        }

        if (received >= '0' && received <= '9' && g_serial_length < (INPUT_BUFFER_SIZE - 1U)) {
            g_serial_buffer[g_serial_length++] = received;
        }
    }
}

void update_control() {
    const unsigned long now = millis();
    if ((now - g_last_control_ms) < CONTROL_PERIOD_MS) {
        return;
    }

    g_last_control_ms = now;
    g_process_value = potentiometer_read();
    g_control_state = control_update(g_set_point, g_process_value);
    apply_outputs(g_control_state);
}

void refresh_serial() {
    const unsigned long now = millis();
    if ((now - g_last_serial_ms) < SERIAL_PERIOD_MS) {
        return;
    }

    g_last_serial_ms = now;
    report_state();
}

void refresh_lcd() {
    const unsigned long now = millis();
    if ((now - g_last_lcd_ms) < LCD_PERIOD_MS) {
        return;
    }

    g_last_lcd_ms = now;
    update_lcd();
}

void refresh_plotter() {
    if (!PLOTTER_OUTPUT_ENABLED) {
        return;
    }

    const unsigned long now = millis();
    if ((now - g_last_plotter_ms) < PLOTTER_PERIOD_MS) {
        return;
    }

    g_last_plotter_ms = now;
    report_plotter_frame();
}

}  // namespace

void setup() {
    Serial.begin(115200);

#ifdef ARDUINO_ARCH_AVR
    fdev_setup_stream(&serial_stdout, serial_putchar, nullptr, _FDEV_SETUP_WRITE);
    stdout = &serial_stdout;
#endif

    motor_init();
    lcd_init();
    lcd_clear();
    potentiometer_init(POTENTIOMETER_PIN);
    control_init(HYSTERESIS_BAND);

    g_process_value = potentiometer_read();
    g_control_state = control_update(g_set_point, g_process_value);
    apply_outputs(g_control_state);
    update_lcd();

    printf("Varianta B: control pozitie rotor cu potentiometru si L298\n");
    printf("Setpoint din interfata seriala: introdu 0..1023 si Enter\n");
    report_state();
}

void loop() {
    process_serial();
    update_control();
    refresh_lcd();
    refresh_serial();
    refresh_plotter();
}
