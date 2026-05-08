#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

static const uint8_t BUTTON_PIN = 2;
static const uint8_t LED1_PIN = 12;
static const uint8_t LED2_PIN = 13;

static const TickType_t TASK1_PERIOD_TICKS = pdMS_TO_TICKS(10);
static const TickType_t DEBOUNCE_TICKS = pdMS_TO_TICKS(30);
static const TickType_t LED1_ON_TICKS = pdMS_TO_TICKS(1000);
static const TickType_t QUEUE_SEND_TICKS = pdMS_TO_TICKS(50);
static const TickType_t LED2_ON_TICKS = pdMS_TO_TICKS(300);
static const TickType_t LED2_OFF_TICKS = pdMS_TO_TICKS(500);
static const TickType_t TASK3_PERIOD_TICKS = pdMS_TO_TICKS(200);

static SemaphoreHandle_t gButtonSemaphore = NULL;
static QueueHandle_t gValueQueue = NULL;

static int gN = 0;

static void TaskButton(void *pvParameters);
static void TaskSync(void *pvParameters);
static void TaskAsync(void *pvParameters);

void setup() {
    Serial.begin(9600);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);

    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);

    gButtonSemaphore = xSemaphoreCreateBinary();
    gValueQueue = xQueueCreate(64, sizeof(uint8_t));

    if (gButtonSemaphore == NULL || gValueQueue == NULL) {
        Serial.println("Init error");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    Serial.println();
    Serial.println("FreeRTOS Mega 2560");
    Serial.println("Button=2 LED1=12 LED2=13");

    xTaskCreate(TaskButton, "TaskButton", 192, NULL, 3, NULL);
    xTaskCreate(TaskSync, "TaskSync", 192, NULL, 2, NULL);
    xTaskCreate(TaskAsync, "TaskAsync", 192, NULL, 1, NULL);
}

void loop() {
}

static void TaskButton(void *pvParameters) {
    (void)pvParameters;

    TickType_t lastWakeTime = xTaskGetTickCount();
    TickType_t lastBounceTime = 0;
    bool lastRawState = false;
    bool stablePressedState = false;

    for (;;) {
        bool rawPressed = (digitalRead(BUTTON_PIN) == LOW);
        TickType_t now = xTaskGetTickCount();

        if (rawPressed != lastRawState) {
            lastRawState = rawPressed;
            lastBounceTime = now;
        }

        if ((now - lastBounceTime) >= DEBOUNCE_TICKS && rawPressed != stablePressedState) {
            stablePressedState = rawPressed;

            if (stablePressedState) {
                Serial.println("[T1] Button pressed");
                digitalWrite(LED1_PIN, HIGH);
                vTaskDelay(LED1_ON_TICKS);
                digitalWrite(LED1_PIN, LOW);
                Serial.println("[T1] LED1 finished");
                xSemaphoreGive(gButtonSemaphore);
                Serial.println("[T1] Semaphore given");
            }
        }

        xTaskDelayUntil(&lastWakeTime, TASK1_PERIOD_TICKS);
    }
}

static void TaskSync(void *pvParameters) {
    (void)pvParameters;

    for (;;) {
        if (xSemaphoreTake(gButtonSemaphore, portMAX_DELAY) == pdTRUE) {
            gN++;
            Serial.print("[T2] Triggered, N=");
            Serial.println(gN);

            for (int i = 1; i <= gN; i++) {
                uint8_t value = (uint8_t)i;
                if (xQueueSend(gValueQueue, &value, portMAX_DELAY) == pdTRUE) {
                    Serial.print("[T2] Queue <- ");
                    Serial.println(value);
                }
                vTaskDelay(QUEUE_SEND_TICKS);
            }

            {
                uint8_t endMarker = 0;
                xQueueSend(gValueQueue, &endMarker, portMAX_DELAY);
            }
            Serial.println("[T2] Queue <- 0");

            for (int i = 0; i < gN; i++) {
                digitalWrite(LED2_PIN, HIGH);
                Serial.println("[T2] LED2 ON");
                vTaskDelay(LED2_ON_TICKS);
                digitalWrite(LED2_PIN, LOW);
                Serial.println("[T2] LED2 OFF");
                vTaskDelay(LED2_OFF_TICKS);
            }

            Serial.println("[T2] Done");
        }
    }
}

static void TaskAsync(void *pvParameters) {
    (void)pvParameters;

    char lineBuffer[128];
    int lineLength = 0;

    lineBuffer[0] = '\0';

    for (;;) {
        uint8_t value = 0;

        while (xQueueReceive(gValueQueue, &value, 0) == pdTRUE) {
            if (value == 0) {
                if (lineLength > 0) {
                    Serial.print("Received: ");
                    Serial.println(lineBuffer);
                } else {
                    Serial.println("Received:");
                }
                lineBuffer[0] = '\0';
                lineLength = 0;
            } else {
                int written = snprintf(
                    lineBuffer + lineLength,
                    sizeof(lineBuffer) - (size_t)lineLength,
                    (lineLength == 0) ? "%u" : " %u",
                    value
                );

                if (written > 0) {
                    lineLength += written;
                    if (lineLength >= (int)sizeof(lineBuffer)) {
                        lineLength = (int)sizeof(lineBuffer) - 1;
                    }
                }
            }
        }

        vTaskDelay(TASK3_PERIOD_TICKS);
    }
}
