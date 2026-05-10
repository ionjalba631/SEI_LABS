#include <Arduino.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "control_pid.h"
#include "lcd_dd.h"
#include "motor_dd.h"
#include "potentiometer_dd.h"

namespace {

enum control_mode_t {
    CONTROL_MODE_AUTO = 0,
    CONTROL_MODE_MANUAL
};

constexpr uint8_t POTENTIOMETER_PIN = A0;
constexpr int POSITION_MIN = 0;
constexpr int POSITION_MAX = 100;
constexpr int DEFAULT_SET_POINT = 50;
constexpr int PWM_LIMIT_PERCENT = 85;
constexpr int DEADBAND_POSITION = 2;
constexpr int SMALL_ERROR_STOP = 1;
constexpr unsigned long CONTROL_PERIOD_MS = 20UL;
constexpr unsigned long LCD_PERIOD_MS = 100UL;
constexpr unsigned long PLOTTER_PERIOD_MS = 100UL;
constexpr size_t INPUT_BUFFER_SIZE = 32U;

constexpr control_pid_config_t PID_CONFIG = {
    2.80f,
    0.35f,
    0.18f,
    -static_cast<float>(PWM_LIMIT_PERCENT),
    static_cast<float>(PWM_LIMIT_PERCENT),
};

char g_serial_buffer[INPUT_BUFFER_SIZE] = "";
uint8_t g_serial_length = 0;
control_mode_t g_control_mode = CONTROL_MODE_AUTO;
int g_set_point = DEFAULT_SET_POINT;
uint16_t g_adc_value = 0;
int g_position = 0;
int g_error = 0;
int g_output_percent = 0;
int g_manual_output_percent = 0;
float g_pid_output_percent = 0.0f;
unsigned long g_last_control_ms = 0;
unsigned long g_last_lcd_ms = 0;
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

int clamp_int(const int value, const int minimum, const int maximum) {
    if (value < minimum) {
        return minimum;
    }

    if (value > maximum) {
        return maximum;
    }

    return value;
}

int absolute_int(const int value) {
    return (value < 0) ? -value : value;
}

int round_to_int(const float value) {
    return static_cast<int>((value >= 0.0f) ? (value + 0.5f) : (value - 0.5f));
}

bool is_blank_character(const char character) {
    return (character == ' ') || (character == '\t');
}

char* skip_leading_blanks(char* text) {
    while (*text != '\0' && is_blank_character(*text)) {
        ++text;
    }

    return text;
}

void trim_trailing_blanks(char* text) {
    size_t length = strlen(text);

    while (length > 0U && is_blank_character(text[length - 1U])) {
        text[length - 1U] = '\0';
        --length;
    }
}

void uppercase_in_place(char* text) {
    while (*text != '\0') {
        *text = static_cast<char>(toupper(static_cast<unsigned char>(*text)));
        ++text;
    }
}

bool starts_with_command(const char* text, const char* command) {
    const size_t command_length = strlen(command);
    return (strncmp(text, command, command_length) == 0) &&
           (text[command_length] == '\0' || is_blank_character(text[command_length]));
}

bool try_parse_int(const char* text, int* value) {
    char* end_ptr = nullptr;
    const long parsed_value = strtol(text, &end_ptr, 10);

    if (text == end_ptr) {
        return false;
    }

    const char* trailing_text = skip_leading_blanks(end_ptr);
    if (*trailing_text != '\0') {
        return false;
    }

    *value = static_cast<int>(parsed_value);
    return true;
}

int adc_to_position(const uint16_t adc_value) {
    const uint32_t span = static_cast<uint32_t>(POSITION_MAX - POSITION_MIN);
    const uint32_t scaled = (static_cast<uint32_t>(adc_value) * span) + 511U;
    return POSITION_MIN + static_cast<int>(scaled / 1023U);
}

const char* mode_to_text() {
    return (g_control_mode == CONTROL_MODE_AUTO) ? "AUTO" : "MANUAL";
}

void update_measurement() {
    g_adc_value = potentiometer_read();
    g_position = clamp_int(adc_to_position(g_adc_value), POSITION_MIN, POSITION_MAX);
    g_error = g_set_point - g_position;
}

void apply_motor_output(const int output_percent) {
    if (output_percent > 0) {
        motor_apply(MOTOR_DD_FORWARD, static_cast<uint8_t>(output_percent));
        return;
    }

    if (output_percent < 0) {
        motor_apply(MOTOR_DD_BACKWARD, static_cast<uint8_t>(-output_percent));
        return;
    }

    motor_stop();
}

void synchronize_pid() {
    control_pid_reset(static_cast<float>(g_position));
    g_pid_output_percent = 0.0f;
}

int apply_output_protections(const int requested_output, const bool is_auto_mode) {
    int protected_output = clamp_int(requested_output, -PWM_LIMIT_PERCENT, PWM_LIMIT_PERCENT);

    if (g_position <= POSITION_MIN && protected_output < 0) {
        protected_output = 0;
    }

    if (g_position >= POSITION_MAX && protected_output > 0) {
        protected_output = 0;
    }

    if (!is_auto_mode) {
        return protected_output;
    }

    const int absolute_error = absolute_int(g_error);
    if (absolute_error <= SMALL_ERROR_STOP) {
        synchronize_pid();
        return 0;
    }

    if (absolute_error <= DEADBAND_POSITION) {
        return 0;
    }

    return protected_output;
}

void print_help() {
    printf("Comenzi: <0..100> | SP <0..100> | AUTO | MAN <-85..85> | STOP | STATUS | HELP\n");
}

void report_state() {
    update_measurement();
    printf(
        "MODE=%s SP=%d POS=%d ERR=%d OUT=%d PID=%d ADC=%u\n",
        mode_to_text(),
        g_set_point,
        g_position,
        g_error,
        g_output_percent,
        round_to_int(g_pid_output_percent),
        g_adc_value
    );
}

void enter_auto_mode() {
    g_control_mode = CONTROL_MODE_AUTO;
    g_manual_output_percent = 0;
    synchronize_pid();
    printf("Mod AUTO activ.\n");
}

void enter_manual_mode(const int output_percent) {
    g_control_mode = CONTROL_MODE_MANUAL;
    g_manual_output_percent = clamp_int(output_percent, -PWM_LIMIT_PERCENT, PWM_LIMIT_PERCENT);
    synchronize_pid();
    printf("Mod MANUAL activ. Output=%d\n", g_manual_output_percent);
}

void update_set_point(const int set_point) {
    g_set_point = clamp_int(set_point, POSITION_MIN, POSITION_MAX);
    g_error = g_set_point - g_position;

    if (g_control_mode == CONTROL_MODE_AUTO) {
        synchronize_pid();
    }

    printf("SetPoint nou: %d\n", g_set_point);
}

void update_lcd() {
    char row0[17];
    char row1[17];

    snprintf(
        row0,
        sizeof(row0),
        "%c SP:%03d P:%03d",
        (g_control_mode == CONTROL_MODE_AUTO) ? 'A' : 'M',
        g_set_point,
        g_position
    );
    snprintf(row1, sizeof(row1), "E:%+04d O:%+04d", g_error, g_output_percent);

    lcd_print_row(0, row0);
    lcd_print_row(1, row1);
}

void report_plotter_frame() {
    printf(
        "SetPoint:%d,Position:%d,Output:%d\n",
        g_set_point,
        g_position,
        g_output_percent
    );
}

void handle_serial_command() {
    g_serial_buffer[g_serial_length] = '\0';
    g_serial_length = 0;

    char* command = skip_leading_blanks(g_serial_buffer);
    trim_trailing_blanks(command);

    if (*command == '\0') {
        return;
    }

    uppercase_in_place(command);

    if (strcmp(command, "AUTO") == 0) {
        enter_auto_mode();
        report_state();
        return;
    }

    if (strcmp(command, "STOP") == 0) {
        enter_manual_mode(0);
        report_state();
        return;
    }

    if (strcmp(command, "STATUS") == 0) {
        report_state();
        return;
    }

    if (strcmp(command, "HELP") == 0) {
        print_help();
        report_state();
        return;
    }

    if (starts_with_command(command, "SP")) {
        int set_point = 0;
        if (!try_parse_int(skip_leading_blanks(command + 2), &set_point)) {
            printf("Comanda SP invalida. Exemplu: SP 50\n");
            return;
        }

        update_set_point(set_point);
        report_state();
        return;
    }

    if (starts_with_command(command, "MANUAL")) {
        int manual_output = 0;
        if (!try_parse_int(skip_leading_blanks(command + 6), &manual_output)) {
            printf("Comanda MANUAL invalida. Exemplu: MANUAL -30\n");
            return;
        }

        enter_manual_mode(manual_output);
        report_state();
        return;
    }

    if (starts_with_command(command, "MAN")) {
        int manual_output = 0;
        if (!try_parse_int(skip_leading_blanks(command + 3), &manual_output)) {
            printf("Comanda MAN invalida. Exemplu: MAN 30\n");
            return;
        }

        enter_manual_mode(manual_output);
        report_state();
        return;
    }

    int numeric_set_point = 0;
    if (try_parse_int(command, &numeric_set_point)) {
        update_set_point(numeric_set_point);
        report_state();
        return;
    }

    printf("Comanda necunoscuta: %s\n", command);
    print_help();
}

void process_serial() {
    while (Serial.available() > 0) {
        const char received = static_cast<char>(Serial.read());

        if (received == '\r') {
            continue;
        }

        if (received == '\n') {
            if (g_serial_length > 0U) {
                handle_serial_command();
            }
            continue;
        }

        if ((received == '\b' || received == 127) && g_serial_length > 0U) {
            --g_serial_length;
            continue;
        }

        if (g_serial_length < (INPUT_BUFFER_SIZE - 1U)) {
            g_serial_buffer[g_serial_length++] = received;
        }
    }
}

int compute_auto_output(const float dt_seconds) {
    const int absolute_error = absolute_int(g_error);
    if (absolute_error <= SMALL_ERROR_STOP) {
        synchronize_pid();
        return 0;
    }

    g_pid_output_percent = control_pid_update(
        static_cast<float>(g_set_point),
        static_cast<float>(g_position),
        dt_seconds
    );

    if (absolute_error <= DEADBAND_POSITION) {
        return 0;
    }

    return round_to_int(g_pid_output_percent);
}

void update_control() {
    const unsigned long now = millis();
    if ((now - g_last_control_ms) < CONTROL_PERIOD_MS) {
        return;
    }

    const float dt_seconds = static_cast<float>(now - g_last_control_ms) / 1000.0f;
    g_last_control_ms = now;
    update_measurement();

    int requested_output = 0;
    if (g_control_mode == CONTROL_MODE_AUTO) {
        requested_output = compute_auto_output(dt_seconds);
    } else {
        g_pid_output_percent = 0.0f;
        requested_output = g_manual_output_percent;
    }

    g_output_percent = apply_output_protections(requested_output, g_control_mode == CONTROL_MODE_AUTO);
    apply_motor_output(g_output_percent);
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
    control_pid_init(PID_CONFIG);

    update_measurement();
    synchronize_pid();
    g_last_control_ms = millis();
    g_last_lcd_ms = g_last_control_ms;
    g_last_plotter_ms = g_last_control_ms;

    apply_motor_output(0);
    update_lcd();

    printf("Control pozitie rotor cu PID, L298N si potentiometru analogic\n");
    printf("Pozitia este mapata in intervalul %d..%d\n", POSITION_MIN, POSITION_MAX);
    print_help();
    report_state();
}

void loop() {
    process_serial();
    update_control();
    refresh_lcd();
    refresh_plotter();
}
