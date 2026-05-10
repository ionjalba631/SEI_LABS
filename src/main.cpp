#include <Arduino.h>
#include <stdio.h>

#include "button_dd.h"
#include "ddLed.h"
#include "lcd_dd.h"
#include "led_fsm.h"

namespace {

constexpr uint8_t APP_LED_PIN = 13;
constexpr uint8_t APP_BUTTON_PIN = 2;
constexpr unsigned long LCD_REFRESH_MS = 100UL;
constexpr unsigned long SERIAL_REFRESH_MS = 250UL;

DdLed g_led = {};
unsigned long g_last_lcd_refresh_ms = 0;
unsigned long g_last_serial_refresh_ms = 0;

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

const char *fsm_state_to_text(const LedFsmState state) {
    switch (state) {
        case LED_FSM_STATE_ON:
            return "ON";
        case LED_FSM_STATE_OFF:
        default:
            return "OFF";
    }
}

void apply_led_state(const LedFsmState state) {
    if (state == LED_FSM_STATE_ON) {
        ddLedOn(&g_led);
        return;
    }

    ddLedOff(&g_led);
}

void update_lcd(const LedFsmState state) {
    char row0[17];
    char row1[17];

    snprintf(row0, sizeof(row0), "FSM LED Control ");
    snprintf(row1, sizeof(row1), "State:%-9s", fsm_state_to_text(state));

    lcd_print_row(0, row0);
    lcd_print_row(1, row1);
}

void report_transition(const LedFsmTransition transition) {
    printf(
        "\n[FSM] Button valid -> %s\n",
        fsm_state_to_text(transition.current_state)
    );
}

void refresh_serial(const LedFsmState state) {
    const unsigned long now = millis();
    if ((now - g_last_serial_refresh_ms) < SERIAL_REFRESH_MS) {
        return;
    }

    g_last_serial_refresh_ms = now;
    printf("\rCurrent FSM state: %-3s", fsm_state_to_text(state));
}

void refresh_lcd(const LedFsmState state) {
    const unsigned long now = millis();
    if ((now - g_last_lcd_refresh_ms) < LCD_REFRESH_MS) {
        return;
    }

    g_last_lcd_refresh_ms = now;
    update_lcd(state);
}

}  // namespace

void setup() {
    Serial.begin(115200);

#ifdef ARDUINO_ARCH_AVR
    fdev_setup_stream(&serial_stdout, serial_putchar, nullptr, _FDEV_SETUP_WRITE);
    stdout = &serial_stdout;
#endif

    ddLedInit(&g_led, APP_LED_PIN);
    button_init(APP_BUTTON_PIN);
    lcd_init();
    lcd_clear();
    led_fsm_init();

    apply_led_state(led_fsm_get_state());
    update_lcd(led_fsm_get_state());

    printf("Lab 6.1 - FSM LED/Button control\n");
    printf("Buton pe pinul 2, LED pe pinul 13\n");
    printf("Stare initiala: %s\n", fsm_state_to_text(led_fsm_get_state()));
}

void loop() {
    const bool pressed_event = button_was_pressed();
    const LedFsmTransition transition = led_fsm_update(pressed_event);

    if (transition.changed) {
        apply_led_state(transition.current_state);
        report_transition(transition);
    }

    refresh_lcd(transition.current_state);
    refresh_serial(transition.current_state);
}
