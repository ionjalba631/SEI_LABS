#include "ctrlRelay.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "ddKeypad.h"
#include "ddLcd.h"
#include "relayDriver.h"

namespace {

constexpr size_t SERIAL_BUFFER_SIZE = 32;

char serial_buffer[SERIAL_BUFFER_SIZE];
size_t serial_length = 0;
bool keypad_key_processed = false;

void lcd_print_fixed_row(const uint8_t row, const char *text) {
    char padded_row[DD_LCD_COLS + 1];
    snprintf(padded_row, sizeof(padded_row), "%-16.16s", text);
    ddLcdSetCursor(0, row);
    ddLcdPrint(padded_row);
}

void show_status_on_lcd(const char *message) {
    lcd_print_fixed_row(0, "Bec prin releu");
    lcd_print_fixed_row(1, message);
}

void print_help() {
    printf("\nCommands: relay on | relay off | status | help\n");
    printf("Controls bulb through relay\n");
    printf("Keypad: A=ON, B=OFF, C=STATUS, D=HELP\n");
}

void report_state(const char *source) {
    const bool relay_is_on = relayDriverIsOn();

    printf(
        "[%s] Bulb is %s\n",
        source,
        relay_is_on ? "ON" : "OFF"
    );

    show_status_on_lcd(relay_is_on ? "Bec: APRINS" : "Bec: STINS");
}

void execute_relay_command(const bool turn_on, const char *source) {
    relayDriverSet(turn_on);

    printf(
        "[%s] Command accepted: relay %s -> bulb %s\n",
        source,
        turn_on ? "on" : "off",
        turn_on ? "ON" : "OFF"
    );

    report_state(source);
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
    for (size_t index = 0; text[index] != '\0'; index++) {
        text[index] = static_cast<char>(tolower(static_cast<unsigned char>(text[index])));
    }
}

void process_text_command(char *command, const char *source) {
    trim_in_place(command);
    to_lowercase_in_place(command);

    if (strcmp(command, "relay on") == 0) {
        execute_relay_command(true, source);
        return;
    }

    if (strcmp(command, "relay off") == 0) {
        execute_relay_command(false, source);
        return;
    }

    if (strcmp(command, "status") == 0) {
        report_state(source);
        return;
    }

    if (strcmp(command, "help") == 0) {
        print_help();
        show_status_on_lcd("A:on B:off C:st");
        return;
    }

    printf("[%s] Invalid command: %s\n", source, command);
    printf("Use: relay on | relay off | status | help\n");
    show_status_on_lcd("Invalid command");
}

void process_serial_input() {
    while (Serial.available() > 0) {
        const char received = static_cast<char>(Serial.read());

        if (received == '\r') {
            continue;
        }

        if (received == '\n') {
            serial_buffer[serial_length] = '\0';

            if (serial_length > 0) {
                process_text_command(serial_buffer, "Serial");
                serial_length = 0;
            }

            continue;
        }

        if (serial_length < (SERIAL_BUFFER_SIZE - 1U)) {
            serial_buffer[serial_length++] = received;
        }
    }
}

void process_keypad_input() {
    const char key = ddKeypadGetKey();

    if (key == DD_KP_NO_KEY) {
        keypad_key_processed = false;
        return;
    }

    if (keypad_key_processed) {
        return;
    }

    keypad_key_processed = true;

    switch (key) {
        case 'A':
            execute_relay_command(true, "Keypad");
            break;

        case 'B':
            execute_relay_command(false, "Keypad");
            break;

        case 'C':
            report_state("Keypad");
            break;

        case 'D':
            print_help();
            show_status_on_lcd("A:on B:off C:st");
            break;

        default:
            printf("[Keypad] Unsupported key: %c\n", key);
            show_status_on_lcd("Use A/B/C/D");
            break;
    }
}

}  // namespace

void ctrlRelayInit() {
    relayDriverInit();
    ddLcdInit();
    ddLcdClear();
    ddKeypadInit();

    show_status_on_lcd("Bec: STINS");
    printf("\nBulb control through relay ready.\n");
    print_help();
    report_state("System");
}

void ctrlRelayUpdate() {
    process_serial_input();
    process_keypad_input();
}

bool ctrlRelayIsOn() {
    return relayDriverIsOn();
}
