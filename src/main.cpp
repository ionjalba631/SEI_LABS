#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <stdio.h>

#include "button_dd.h"
#include "ddLed.h"
#include "lcd_dd.h"
#include "traffic_fsm.h"

namespace {

constexpr uint8_t NORTH_REQUEST_BUTTON_PIN = 2;

constexpr uint8_t EAST_RED_PIN = 13;
constexpr uint8_t EAST_YELLOW_PIN = 12;
constexpr uint8_t EAST_GREEN_PIN = 11;

constexpr uint8_t NORTH_RED_PIN = 10;
constexpr uint8_t NORTH_YELLOW_PIN = 9;
constexpr uint8_t NORTH_GREEN_PIN = 8;

constexpr TickType_t SENSOR_TASK_PERIOD = pdMS_TO_TICKS(40);
constexpr TickType_t CONTROLLER_TASK_PERIOD = pdMS_TO_TICKS(100);
constexpr TickType_t DISPLAY_TASK_PERIOD = pdMS_TO_TICKS(500);
constexpr bool SERIAL_JSON_LOGGING = false;

DdLed g_east_red = {};
DdLed g_east_yellow = {};
DdLed g_east_green = {};
DdLed g_north_red = {};
DdLed g_north_yellow = {};
DdLed g_north_green = {};

SemaphoreHandle_t g_north_request_semaphore = nullptr;
TrafficSnapshot g_snapshot = {};
TrafficPhase g_last_reported_phase = TRAFFIC_PHASE_EAST_GREEN;

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

const char *lamp_to_text(const TrafficLampState lamp) {
    switch (lamp) {
        case TRAFFIC_LAMP_RED:
            return "RED";
        case TRAFFIC_LAMP_YELLOW:
            return "YELLOW";
        case TRAFFIC_LAMP_GREEN:
            return "GREEN";
        default:
            return "UNKNOWN";
    }
}

const char *phase_to_text(const TrafficPhase phase) {
    switch (phase) {
        case TRAFFIC_PHASE_EAST_GREEN:
            return "EAST_GREEN";
        case TRAFFIC_PHASE_EAST_YELLOW:
            return "EAST_YELLOW";
        case TRAFFIC_PHASE_ALL_RED_TO_NORTH:
            return "ALL_RED_TO_NORTH";
        case TRAFFIC_PHASE_NORTH_GREEN:
            return "NORTH_GREEN";
        case TRAFFIC_PHASE_NORTH_YELLOW:
            return "NORTH_YELLOW";
        case TRAFFIC_PHASE_ALL_RED_TO_EAST:
            return "ALL_RED_TO_EAST";
        default:
            return "UNKNOWN";
    }
}

void set_led(DdLed *led, const bool on) {
    if (on) {
        ddLedOn(led);
        return;
    }

    ddLedOff(led);
}

void apply_snapshot_to_leds(const TrafficSnapshot &snapshot) {
    set_led(&g_east_red, snapshot.east.red);
    set_led(&g_east_yellow, snapshot.east.yellow);
    set_led(&g_east_green, snapshot.east.green);

    set_led(&g_north_red, snapshot.north.red);
    set_led(&g_north_yellow, snapshot.north.yellow);
    set_led(&g_north_green, snapshot.north.green);
}

void update_lcd(const TrafficSnapshot &snapshot) {
    char row0[17];
    char row1[17];

    snprintf(
        row0,
        sizeof(row0),
        "E:%-6s N:%-6s",
        lamp_to_text(snapshot.east.lamp),
        lamp_to_text(snapshot.north.lamp)
    );
    snprintf(
        row1,
        sizeof(row1),
        "Req:%c T:%2lus",
        snapshot.north_request_pending ? 'Y' : 'N',
        static_cast<unsigned long>(snapshot.phase_elapsed_ms / 1000UL)
    );

    lcd_print_row(0, row0);
    lcd_print_row(1, row1);
}

void print_snapshot_json(const TrafficSnapshot &snapshot) {
    printf(
        "{\"phase\":\"%s\",\"east\":\"%s\",\"north\":\"%s\",\"northRequestPending\":%s,"
        "\"phaseElapsedMs\":%lu,\"phaseDurationMs\":%lu}\n",
        phase_to_text(snapshot.phase),
        lamp_to_text(snapshot.east.lamp),
        lamp_to_text(snapshot.north.lamp),
        snapshot.north_request_pending ? "true" : "false",
        static_cast<unsigned long>(snapshot.phase_elapsed_ms),
        static_cast<unsigned long>(snapshot.phase_duration_ms)
    );
}

void print_serial_header() {
    printf("\n%-18s %-8s %-8s %-4s %-9s\n", "PHASE", "EAST", "NORTH", "REQ", "TIMER");
    printf("----------------------------------------------------\n");
}

void refresh_serial_line(const TrafficSnapshot &snapshot) {
    printf(
        "\r%-18s %-8s %-8s %-4c %2lu/%2lus ",
        phase_to_text(snapshot.phase),
        lamp_to_text(snapshot.east.lamp),
        lamp_to_text(snapshot.north.lamp),
        snapshot.north_request_pending ? 'Y' : 'N',
        static_cast<unsigned long>(snapshot.phase_elapsed_ms / 1000UL),
        static_cast<unsigned long>(snapshot.phase_duration_ms / 1000UL)
    );
}

void copy_snapshot(TrafficSnapshot *destination) {
    taskENTER_CRITICAL();
    *destination = g_snapshot;
    taskEXIT_CRITICAL();
}

void store_snapshot(const TrafficSnapshot &snapshot) {
    taskENTER_CRITICAL();
    g_snapshot = snapshot;
    taskEXIT_CRITICAL();
}

void sensor_task(void *parameter) {
    (void)parameter;

    TickType_t last_wake_time = xTaskGetTickCount();

    for (;;) {
        if (button_was_pressed()) {
            xSemaphoreGive(g_north_request_semaphore);
        }

        vTaskDelayUntil(&last_wake_time, SENSOR_TASK_PERIOD);
    }
}

void controller_task(void *parameter) {
    (void)parameter;

    TickType_t last_wake_time = xTaskGetTickCount();
    TrafficFsmContext fsm = {};
    bool north_request_pending = false;

    traffic_fsm_init(&fsm);
    store_snapshot(traffic_fsm_get_snapshot(&fsm, north_request_pending));
    apply_snapshot_to_leds(traffic_fsm_get_snapshot(&fsm, north_request_pending));

    for (;;) {
        if (xSemaphoreTake(g_north_request_semaphore, 0) == pdTRUE) {
            north_request_pending = true;
        }

        traffic_fsm_tick(&fsm, north_request_pending, pdTICKS_TO_MS(CONTROLLER_TASK_PERIOD));

        if (traffic_fsm_should_clear_request(&fsm)) {
            north_request_pending = false;
        }

        const TrafficSnapshot snapshot = traffic_fsm_get_snapshot(&fsm, north_request_pending);
        store_snapshot(snapshot);
        apply_snapshot_to_leds(snapshot);

        vTaskDelayUntil(&last_wake_time, CONTROLLER_TASK_PERIOD);
    }
}

void display_task(void *parameter) {
    (void)parameter;

    TickType_t last_wake_time = xTaskGetTickCount();
    TrafficSnapshot snapshot = {};

    for (;;) {
        copy_snapshot(&snapshot);
        update_lcd(snapshot);
        refresh_serial_line(snapshot);

        if (snapshot.phase != g_last_reported_phase) {
            printf("\n");
            if (SERIAL_JSON_LOGGING) {
                print_snapshot_json(snapshot);
            }
            g_last_reported_phase = snapshot.phase;
            print_serial_header();
        }

        vTaskDelayUntil(&last_wake_time, DISPLAY_TASK_PERIOD);
    }
}

void init_hardware() {
    ddLedInit(&g_east_red, EAST_RED_PIN);
    ddLedInit(&g_east_yellow, EAST_YELLOW_PIN);
    ddLedInit(&g_east_green, EAST_GREEN_PIN);
    ddLedInit(&g_north_red, NORTH_RED_PIN);
    ddLedInit(&g_north_yellow, NORTH_YELLOW_PIN);
    ddLedInit(&g_north_green, NORTH_GREEN_PIN);

    button_init(NORTH_REQUEST_BUTTON_PIN);
    lcd_init();
    lcd_clear();
}

}  // namespace

void setup() {
    Serial.begin(115200);

#ifdef ARDUINO_ARCH_AVR
    fdev_setup_stream(&serial_stdout, serial_putchar, nullptr, _FDEV_SETUP_WRITE);
    stdout = &serial_stdout;
#endif

    init_hardware();

    g_north_request_semaphore = xSemaphoreCreateBinary();

    TrafficFsmContext initial_fsm = {};
    traffic_fsm_init(&initial_fsm);
    g_snapshot = traffic_fsm_get_snapshot(&initial_fsm, false);
    apply_snapshot_to_leds(g_snapshot);
    update_lcd(g_snapshot);

    printf("Smart traffic light controller starting...\n");
    printf("North request button on pin %u\n", NORTH_REQUEST_BUTTON_PIN);
    print_serial_header();

    xTaskCreate(sensor_task, "sensor", 192, nullptr, 2, nullptr);
    xTaskCreate(controller_task, "traffic", 256, nullptr, 3, nullptr);
    xTaskCreate(display_task, "display", 256, nullptr, 1, nullptr);
}

void loop() {
}
