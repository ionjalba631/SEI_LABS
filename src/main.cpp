#include <Arduino.h>
#include <stdio.h>

#include "ctrlStdio.h"
#include "ddButton.h"
#include "ddLed.h"

static const uint8_t BUTTON_LED_TOGGLE_PIN = 2;
static const uint8_t BUTTON_INCREMENT_PIN  = 3;
static const uint8_t BUTTON_DECREMENT_PIN  = 4;
static const uint8_t LED1_PIN              = 12;
static const uint8_t LED2_PIN              = 11;

static const uint32_t TASK1_PERIOD_MS = 20;
static const uint32_t TASK2_PERIOD_MS = 100;
static const uint32_t TASK3_PERIOD_MS = 20;

static const uint32_t TASK1_OFFSET_MS = 0;
static const uint32_t TASK2_OFFSET_MS = 5;
static const uint32_t TASK3_OFFSET_MS = 10;

static const uint32_t IDLE_DELAY_MS = 5;

static const int MIN_LED2_RECURRENCES     = 1;
static const int MAX_LED2_RECURRENCES     = 10;
static const int DEFAULT_LED2_RECURRENCES = 3;

typedef struct {
    uint32_t periodMs;
    uint32_t offsetMs;
    uint32_t lastReleaseMs;
    bool started;
} TaskSchedule;

static DdButton gButtonLedToggle;
static DdButton gButtonIncrement;
static DdButton gButtonDecrement;
static DdLed gLed1;
static DdLed gLed2;

static bool gLed1Enabled             = false;
static bool gLed2Enabled             = false;
static uint8_t gLed2StateRecurrences = 0;
static int gLed2RecurrenceTarget     = DEFAULT_LED2_RECURRENCES;
static bool gStateChangedForReport   = true;
static uint32_t gSchedulerStartMs    = 0;

static TaskSchedule gTask1Schedule = {TASK1_PERIOD_MS, TASK1_OFFSET_MS, 0, false};
static TaskSchedule gTask2Schedule = {TASK2_PERIOD_MS, TASK2_OFFSET_MS, 0, false};
static TaskSchedule gTask3Schedule = {TASK3_PERIOD_MS, TASK3_OFFSET_MS, 0, false};

static bool shouldRunTask(TaskSchedule *schedule, uint32_t nowMs);
static void runTask1ButtonLed(void);
static void runTask2BlinkingLed(void);
static void runTask3StateVariable(void);
static void runIdleReporter(void);
static void syncLedHardware(void);
static void requestReport(void);

void setup() {
    ctrlStdioInit();

    ddButtonInit(&gButtonLedToggle, BUTTON_LED_TOGGLE_PIN);
    ddButtonInit(&gButtonIncrement, BUTTON_INCREMENT_PIN);
    ddButtonInit(&gButtonDecrement, BUTTON_DECREMENT_PIN);
    ddLedInit(&gLed1, LED1_PIN);
    ddLedInit(&gLed2, LED2_PIN);

    gSchedulerStartMs = millis();

    printf("\nSequential MCU scheduler ready\n");
    printf("BTN toggle=%u, BTN inc=%u, BTN dec=%u, LED1=%u, LED2=%u\n",
           BUTTON_LED_TOGGLE_PIN,
           BUTTON_INCREMENT_PIN,
           BUTTON_DECREMENT_PIN,
           LED1_PIN,
           LED2_PIN);
    requestReport();
}

void loop() {
    const uint32_t nowMs = millis();

    if (shouldRunTask(&gTask1Schedule, nowMs)) {
        runTask1ButtonLed();
    }

    if (shouldRunTask(&gTask2Schedule, nowMs)) {
        runTask2BlinkingLed();
    }

    if (shouldRunTask(&gTask3Schedule, nowMs)) {
        runTask3StateVariable();
    }

    runIdleReporter();
}

static bool shouldRunTask(TaskSchedule *schedule, uint32_t nowMs) {
    if (schedule == NULL) {
        return false;
    }

    if ((nowMs - gSchedulerStartMs) < schedule->offsetMs) {
        return false;
    }

    if (!schedule->started) {
        schedule->started = true;
        schedule->lastReleaseMs = nowMs;
        return true;
    }

    if ((nowMs - schedule->lastReleaseMs) >= schedule->periodMs) {
        schedule->lastReleaseMs += schedule->periodMs;

        if ((nowMs - schedule->lastReleaseMs) >= schedule->periodMs) {
            schedule->lastReleaseMs = nowMs;
        }

        return true;
    }

    return false;
}

static void runTask1ButtonLed(void) {
    ddButtonUpdate(&gButtonLedToggle);

    if (ddButtonWasPressed(&gButtonLedToggle) != 0U) {
        gLed1Enabled = !gLed1Enabled;

        if (gLed1Enabled) {
            gLed2Enabled = false;
            gLed2StateRecurrences = 0;
        }

        requestReport();
        syncLedHardware();
    }
}

static void runTask2BlinkingLed(void) {
    if (gLed1Enabled) {
        if (gLed2Enabled) {
            gLed2Enabled = false;
            gLed2StateRecurrences = 0;
            requestReport();
            syncLedHardware();
        }
        return;
    }

    gLed2StateRecurrences++;

    if (gLed2StateRecurrences >= (uint8_t)gLed2RecurrenceTarget) {
        gLed2StateRecurrences = 0;
        gLed2Enabled = !gLed2Enabled;
        syncLedHardware();
    }
}

static void runTask3StateVariable(void) {
    ddButtonUpdate(&gButtonIncrement);
    ddButtonUpdate(&gButtonDecrement);

    if (ddButtonWasPressed(&gButtonIncrement) != 0U &&
        gLed2RecurrenceTarget < MAX_LED2_RECURRENCES) {
        gLed2RecurrenceTarget++;
        requestReport();
    }

    if (ddButtonWasPressed(&gButtonDecrement) != 0U &&
        gLed2RecurrenceTarget > MIN_LED2_RECURRENCES) {
        gLed2RecurrenceTarget--;
        requestReport();
    }
}

static void runIdleReporter(void) {
    if (gStateChangedForReport) {
        gStateChangedForReport = false;

        printf("[IDLE] LED1=%s | LED2=%s | RecurrenceTarget=%d | CurrentRecurrence=%u\n",
               gLed1Enabled ? "ON " : "OFF",
               gLed2Enabled ? "ON " : "OFF",
               gLed2RecurrenceTarget,
               gLed2StateRecurrences);
    }

    delay(IDLE_DELAY_MS);
}

static void requestReport(void) {
    gStateChangedForReport = true;
}

static void syncLedHardware(void) {
    if (gLed1Enabled) {
        ddLedOn(&gLed1);
    } else {
        ddLedOff(&gLed1);
    }

    if (gLed2Enabled) {
        ddLedOn(&gLed2);
    } else {
        ddLedOff(&gLed2);
    }
}
