#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <stdio.h>

#include "ddLcd.h"
#include "joystick_driver.h"
#include "system_signals.h"

namespace {

constexpr TickType_t ACQUISITION_PERIOD_TICKS = pdMS_TO_TICKS(50);
constexpr TickType_t DISPLAY_PERIOD_TICKS = pdMS_TO_TICKS(500);
constexpr TickType_t DISPLAY_START_OFFSET_TICKS = pdMS_TO_TICKS(25);

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

void lcd_print_fixed_row(const uint8_t row, const char *text) {
    char padded_row[DD_LCD_COLS + 1];
    snprintf(padded_row, sizeof(padded_row), "%-16.16s", text);
    ddLcdSetCursor(0, row);
    ddLcdPrint(padded_row);
}

void lcd_show_position_page(const system_state_t &state) {
    char lcd_row_0[DD_LCD_COLS + 1];
    char lcd_row_1[DD_LCD_COLS + 1];

    snprintf(
        lcd_row_0,
        sizeof(lcd_row_0),
        "X:%4d Y:%4d",
        state.x.position_percent,
        state.y.position_percent
    );
    snprintf(
        lcd_row_1,
        sizeof(lcd_row_1),
        "B:%s S:%s",
        state.button_pressed ? "ON" : "OFF",
        state.error ? "ON" : "OFF"
    );

    lcd_print_fixed_row(0, lcd_row_0);
    lcd_print_fixed_row(1, lcd_row_1);
}

void lcd_show_voltage_page(const system_state_t &state) {
    char lcd_row_0[DD_LCD_COLS + 1];
    char lcd_row_1[DD_LCD_COLS + 1];

    snprintf(lcd_row_0, sizeof(lcd_row_0), "XU:%4umV", state.x.voltage_mv);
    snprintf(lcd_row_1, sizeof(lcd_row_1), "YU:%4umV", state.y.voltage_mv);

    lcd_print_fixed_row(0, lcd_row_0);
    lcd_print_fixed_row(1, lcd_row_1);
}

void lcd_show_adc_page(const system_state_t &state) {
    char lcd_row_0[DD_LCD_COLS + 1];
    char lcd_row_1[DD_LCD_COLS + 1];

    snprintf(lcd_row_0, sizeof(lcd_row_0), "XR:%4u XF:%4u", state.x.raw_adc, state.x.weighted_filtered_adc);
    snprintf(lcd_row_1, sizeof(lcd_row_1), "YR:%4u YF:%4u", state.y.raw_adc, state.y.weighted_filtered_adc);

    lcd_print_fixed_row(0, lcd_row_0);
    lcd_print_fixed_row(1, lcd_row_1);
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

    TickType_t last_wake_time = xTaskGetTickCount();
    uint8_t lcd_page = 0;

    for (;;) {
        const system_state_t state = system_get_state();

        printf(
            "\rX raw:%4u med:%4u avg:%4u U:%4umV P:%4d%% | "
            "Y raw:%4u med:%4u avg:%4u U:%4umV P:%4d%% | "
            "BTN:%-8s | SAT:%-3s   ",
            state.x.raw_adc,
            state.x.salt_pepper_filtered_adc,
            state.x.weighted_filtered_adc,
            state.x.voltage_mv,
            state.x.position_percent,
            state.y.raw_adc,
            state.y.salt_pepper_filtered_adc,
            state.y.weighted_filtered_adc,
            state.y.voltage_mv,
            state.y.position_percent,
            state.button_pressed ? "PRESSED" : "RELEASED",
            state.error ? "YES" : "NO"
        );

        switch (lcd_page) {
            case 0:
                lcd_show_position_page(state);
                break;

            case 1:
                lcd_show_voltage_page(state);
                break;

            default:
                lcd_show_adc_page(state);
                break;
        }

        lcd_page = (lcd_page + 1U) % 3U;

        vTaskDelayUntil(&last_wake_time, DISPLAY_PERIOD_TICKS);
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
    lcd_print_fixed_row(0, "Joystick Monitor");
    lcd_print_fixed_row(1, "Init...");
    joystick_init();
    system_init();
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
    // Executia este preluata de schedulerul FreeRTOS.
}
