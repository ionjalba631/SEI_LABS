#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <stdio.h>


#include "ddLcd.h"
#include "joystick_driver.h"
#include "system_signals.h"

namespace {

constexpr TickType_t ACQUISITION_PERIOD_TICKS = pdMS_TO_TICKS(100);
constexpr TickType_t DISPLAY_PERIOD_TICKS = pdMS_TO_TICKS(500);
constexpr TickType_t DISPLAY_START_OFFSET_TICKS = pdMS_TO_TICKS(50);

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

void lcd_print_fixed_row(uint8_t row, const char *text) {
    ddLcdSetCursor(0, row);
    ddLcdPrint(text);
}

void acquisition_task(void *parameters) {
    (void)parameters;

    TickType_t last_wake_time = xTaskGetTickCount();

    for (;;) {
        system_update_state();
        vTaskDelayUntil(&last_wake_time, ACQUISITION_PERIOD_TICKS);
    }
}

void display_task(void *parameters) {
    (void)parameters;

    vTaskDelay(DISPLAY_START_OFFSET_TICKS);

    for (;;) {
        const system_state_t state = system_get_state();
        char lcd_row_0[DD_LCD_COLS + 1];
        char lcd_row_1[DD_LCD_COLS + 1];

        printf(
            "\rX: %4d%% | Y: %4d%% | Button: %-8s | Error: %-3s      ",
            state.x,
            state.y,
            state.button_pressed ? "PRESSED" : "RELEASED",
            state.error ? "YES" : "NO"
        );

        snprintf(lcd_row_0, sizeof(lcd_row_0), "X:%4d Y:%4d   ", state.x, state.y);
        snprintf(
            lcd_row_1,
            sizeof(lcd_row_1),
            "B:%-3s E:%-3s   ",
            state.button_pressed ? "YES" : "NO",
            state.error ? "YES" : "NO"
        );

        lcd_print_fixed_row(0, lcd_row_0);
        lcd_print_fixed_row(1, lcd_row_1);

        vTaskDelay(DISPLAY_PERIOD_TICKS);
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);

#ifdef ARDUINO_ARCH_AVR
    fdev_setup_stream(&serial_stdout, serial_putchar, nullptr, _FDEV_SETUP_WRITE);
    stdout = &serial_stdout;
#endif

    ddLcdInit();
    ddLcdClear();
    joystick_init();
    system_update_state();

    xTaskCreate(
        acquisition_task,
        "Acquire",
        256,
        nullptr,
        2,
        nullptr
    );

    xTaskCreate(
        display_task,
        "Display",
        256,
        nullptr,
        1,
        nullptr
    );
}

void loop() {
    // FreeRTOS preia executia task-urilor; loop ramane gol.
}
