#include "ctrlMotor.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ddKeypad.h"
#include "ddLcd.h"
#include "ddMotorL298.h"

namespace {

constexpr size_t SERIAL_BUFFER_SIZE = 48;
constexpr size_t TERMINAL_BUFFER_SIZE = 48;
constexpr unsigned long STATUS_REPORT_PERIOD_MS = 3000UL;
constexpr int8_t MOTOR_POWER_MIN = -100;
constexpr int8_t MOTOR_POWER_MAX = 100;
constexpr int8_t MOTOR_STEP_PERCENT = 10;

struct motor_state_t {
    int8_t signed_power_percent;
    int8_t last_direction_sign;
};

char g_serial_buffer[SERIAL_BUFFER_SIZE];
size_t g_serial_length = 0;
bool g_keypad_key_processed = false;
unsigned long g_last_report_ms = 0;
motor_state_t g_motor_state = {0, 1};
char g_last_ui_message[TERMINAL_BUFFER_SIZE] = "System ready";

int8_t clamp_power(const int value) {
    if (value < MOTOR_POWER_MIN) {
        return MOTOR_POWER_MIN;
    }

    if (value > MOTOR_POWER_MAX) {
        return MOTOR_POWER_MAX;
    }

    return static_cast<int8_t>(value);
}

const char *direction_to_text(const int8_t signed_power_percent) {
    if (signed_power_percent > 0) {
        return "FWD";
    }

    if (signed_power_percent < 0) {
        return "REV";
    }

    return "STOP";
}

dd_motor_direction_t signed_power_to_direction(const int8_t signed_power_percent) {
    if (signed_power_percent > 0) {
        return DD_MOTOR_DIRECTION_FORWARD;
    }

    if (signed_power_percent < 0) {
        return DD_MOTOR_DIRECTION_REVERSE;
    }

    return DD_MOTOR_DIRECTION_STOP;
}

uint8_t signed_power_to_magnitude(const int8_t signed_power_percent) {
    if (signed_power_percent < 0) {
        return static_cast<uint8_t>(-signed_power_percent);
    }

    return static_cast<uint8_t>(signed_power_percent);
}

void lcd_print_fixed_row(const uint8_t row, const char *text) {
    char padded_row[DD_LCD_COLS + 1];
    snprintf(padded_row, sizeof(padded_row), "%-16.16s", text);
    ddLcdSetCursor(0, row);
    ddLcdPrint(padded_row);
}

void update_lcd(const char *message) {
    char state_row[DD_LCD_COLS + 1];
    snprintf(
        state_row,
        sizeof(state_row),
        "M:%s %3d%%",
        direction_to_text(g_motor_state.signed_power_percent),
        signed_power_to_magnitude(g_motor_state.signed_power_percent)
    );

    lcd_print_fixed_row(0, state_row);
    lcd_print_fixed_row(1, message);
}

void set_ui_message(const char *message) {
    snprintf(g_last_ui_message, sizeof(g_last_ui_message), "%s", message);
}

void terminal_print_line(const char *text) {
    Serial.print(F("\x1b[2K"));
    Serial.print(text);
    Serial.print(F("\r\n"));
}

void update_terminal() {
    char state_line[TERMINAL_BUFFER_SIZE];
    char message_line[TERMINAL_BUFFER_SIZE];

    snprintf(
        state_line,
        sizeof(state_line),
        "Motor: %-4s %3u%%",
        direction_to_text(g_motor_state.signed_power_percent),
        static_cast<unsigned int>(signed_power_to_magnitude(g_motor_state.signed_power_percent))
    );

    snprintf(message_line, sizeof(message_line), "Mesaj: %.32s", g_last_ui_message);

    Serial.print(F("\x1b[H"));
    terminal_print_line("DC motor control");
    terminal_print_line(state_line);
    terminal_print_line(message_line);
    terminal_print_line("Comenzi: set stop max inc dec");
    Serial.print(F("\x1b[J"));
}

void update_ui(const char *message) {
    set_ui_message(message);
    update_lcd(message);
    update_terminal();
}

void apply_motor_state() {
    const int8_t signed_power = g_motor_state.signed_power_percent;
    if (signed_power > 0) {
        g_motor_state.last_direction_sign = 1;
    } else if (signed_power < 0) {
        g_motor_state.last_direction_sign = -1;
    }

    ddMotorL298Apply(
        signed_power_to_direction(signed_power),
        signed_power_to_magnitude(signed_power)
    );
}

void report_state(const char *source, const bool force = false) {
    const unsigned long now = millis();
    if (!force && g_serial_length > 0U) {
        return;
    }

    if (!force && (now - g_last_report_ms) < STATUS_REPORT_PERIOD_MS) {
        return;
    }

    g_last_report_ms = now;
    (void)source;
    update_lcd(g_last_ui_message);
    update_terminal();
}

void set_power(const int8_t signed_power_percent, const char *source, const char *message) {
    g_motor_state.signed_power_percent = clamp_power(signed_power_percent);
    apply_motor_state();
    (void)source;
    update_ui(message);
    report_state(source, true);
}

void print_help() {
    update_ui("Vezi comenzile");
}

void trim_in_place(char *text) {
    size_t start = 0;
    while (text[start] != '\0' && isspace(static_cast<unsigned char>(text[start])) != 0) {
        start++;
    }

    size_t end = strlen(text);
    while (end > start && isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        end--;
    }

    if (start > 0) {
        memmove(text, text + start, end - start);
    }

    text[end - start] = '\0';
}

void to_lowercase_in_place(char *text) {
    for (size_t index = 0; text[index] != '\0'; ++index) {
        text[index] = static_cast<char>(tolower(static_cast<unsigned char>(text[index])));
    }
}

bool parse_set_command(const char *command, int &value) {
    int parsed_value = 0;
    char trailing = '\0';
    const int match_count = sscanf(command, "motor set %d %c", &parsed_value, &trailing);

    if (match_count != 1) {
        return false;
    }

    if (parsed_value < MOTOR_POWER_MIN || parsed_value > MOTOR_POWER_MAX) {
        return false;
    }

    value = parsed_value;
    return true;
}

void increase_power(const char *source) {
    if (g_motor_state.signed_power_percent == 0) {
        set_power(static_cast<int8_t>(g_motor_state.last_direction_sign * MOTOR_STEP_PERCENT), source, "Increment");
        return;
    }

    if (g_motor_state.signed_power_percent > 0) {
        set_power(static_cast<int8_t>(g_motor_state.signed_power_percent + MOTOR_STEP_PERCENT), source, "Increment");
        return;
    }

    set_power(static_cast<int8_t>(g_motor_state.signed_power_percent - MOTOR_STEP_PERCENT), source, "Increment");
}

int8_t step_toward_zero(const int8_t signed_power_percent) {
    if (signed_power_percent > 0) {
        const int reduced_power = signed_power_percent - MOTOR_STEP_PERCENT;
        return static_cast<int8_t>(reduced_power > 0 ? reduced_power : 0);
    }

    if (signed_power_percent < 0) {
        const int reduced_power = signed_power_percent + MOTOR_STEP_PERCENT;
        return static_cast<int8_t>(reduced_power < 0 ? reduced_power : 0);
    }

    return 0;
}

void decrease_power(const char *source) {
    set_power(step_toward_zero(g_motor_state.signed_power_percent), source, "Decrement");
}

void set_max_power(const char *source) {
    const int8_t direction_sign = (g_motor_state.signed_power_percent < 0) ? -1 : g_motor_state.last_direction_sign;
    set_power(static_cast<int8_t>(direction_sign * MOTOR_POWER_MAX), source, "Set max");
}

void reverse_direction(const char *source) {
    if (g_motor_state.signed_power_percent == 0) {
        g_motor_state.last_direction_sign *= -1;
        (void)source;
        update_ui(g_motor_state.last_direction_sign > 0 ? "Directie: FWD" : "Directie: REV");
        return;
    }

    set_power(static_cast<int8_t>(-g_motor_state.signed_power_percent), source, "Reverse");
}

void process_text_command(char *command, const char *source) {
    trim_in_place(command);
    to_lowercase_in_place(command);

    int set_value = 0;
    if (parse_set_command(command, set_value)) {
        set_power(static_cast<int8_t>(set_value), source, "Set power");
        return;
    }

    if (strcmp(command, "motor stop") == 0) {
        set_power(0, source, "Motor stop");
        return;
    }

    if (strcmp(command, "motor max") == 0) {
        set_max_power(source);
        return;
    }

    if (strcmp(command, "motor inc") == 0) {
        increase_power(source);
        return;
    }

    if (strcmp(command, "motor dec") == 0) {
        decrease_power(source);
        return;
    }

    if (strcmp(command, "status") == 0 || strcmp(command, "motor status") == 0) {
        set_ui_message("State reported");
        report_state(source, true);
        return;
    }

    if (strcmp(command, "help") == 0) {
        print_help();
        return;
    }

    (void)source;
    update_ui("Comanda invalida");
}

void process_const_command(const char *command, const char *source) {
    char command_buffer[SERIAL_BUFFER_SIZE];
    snprintf(command_buffer, sizeof(command_buffer), "%s", command);
    process_text_command(command_buffer, source);
}

void process_set_power_command(const int8_t signed_power_percent, const char *source) {
    char command_buffer[SERIAL_BUFFER_SIZE];
    snprintf(command_buffer, sizeof(command_buffer), "motor set %d", signed_power_percent);
    process_text_command(command_buffer, source);
}

void process_serial_input() {
    while (Serial.available() > 0) {
        const char received = static_cast<char>(Serial.read());

        if (received == '\r') {
            continue;
        }

        if (received == '\n') {
            g_serial_buffer[g_serial_length] = '\0';
            if (g_serial_length > 0U) {
                process_text_command(g_serial_buffer, "Serial");
                g_serial_length = 0;
            }
            continue;
        }

        if (g_serial_length < (SERIAL_BUFFER_SIZE - 1U)) {
            g_serial_buffer[g_serial_length++] = received;
        }
    }
}

void process_keypad_input() {
    const char key = ddKeypadGetKey();

    if (key == DD_KP_NO_KEY) {
        g_keypad_key_processed = false;
        return;
    }

    if (g_keypad_key_processed) {
        return;
    }

    g_keypad_key_processed = true;

    switch (key) {
        case 'A':
            process_const_command("motor inc", "Keypad");
            break;

        case 'B':
            process_const_command("motor dec", "Keypad");
            break;

        case 'C':
            process_const_command("motor max", "Keypad");
            break;

        case 'D':
            process_const_command("motor stop", "Keypad");
            break;

        case '#':
            process_const_command("motor status", "Keypad");
            break;

        case '*':
            reverse_direction("Keypad");
            break;

        case '0':
            process_set_power_command(0, "Keypad");
            break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            const int8_t magnitude = static_cast<int8_t>((key - '0') * 10);
            process_set_power_command(
                static_cast<int8_t>(g_motor_state.last_direction_sign * magnitude),
                "Keypad"
            );
            break;
        }

        default:
            printf("[Keypad] Unsupported key: %c\n", key);
            update_lcd("Use keypad help");
            break;
    }
}

}  // namespace

void ctrlMotorInit() {
    ddLcdInit();
    ddLcdClear();
    ddKeypadInit();
    ddMotorL298Init();

    Serial.print(F("\x1b[2J\x1b[H"));
    update_ui("System ready");
    report_state("System", true);
}

void ctrlMotorUpdate() {
    process_serial_input();
    process_keypad_input();
    report_state("Periodic");
}
