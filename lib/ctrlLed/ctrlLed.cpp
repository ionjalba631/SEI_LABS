#include "ctrlLed.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static DdLed mainLed;

static const char *cmdStrings[CMD_COUNT] = {
    "led on",
    "led off"
};

static void (*cmdActions[CMD_COUNT])(DdLed*) = {
    ddLedOn,
    ddLedOff
};

void ctrlLedInit(void) {
    // Initializarea se face acum pe pinul 13
    ddLedInit(&mainLed, APP_LED_PIN);
    printf("Sistem Mega 2560 Ready.\nComenzi: 'led on' / 'led off'\n");
}

void ctrlLedUpdate(void) {
    static char buffer[CMD_BUFFER_SIZE];
    static int bufPos = 0;

    // Citire non-blocking caracter cu caracter
    while (Serial.available() > 0 && bufPos < CMD_BUFFER_SIZE - 1) {
        char c = Serial.read();
        
        // Handle newline (ambele \n si \r\n)
        if (c == '\r' || c == '\n') {
            if (bufPos > 0) {
                buffer[bufPos] = '\0';
                
                // Trim spaces
                char *cmdPtr = buffer;
                while (*cmdPtr == ' ') cmdPtr++;
                
                // Execute command
                bool recognized = false;
                for (int i = 0; i < CMD_COUNT; i++) {
                    if (strcmp(cmdPtr, cmdStrings[i]) == 0) {
                        cmdActions[i](&mainLed);
                        printf("Succes: [%s] executata.\n", cmdPtr);
                        recognized = true;
                        break;
                    }
                }
                
                if (!recognized && strlen(cmdPtr) > 0) {
                    printf("Eroare: Comanda '%s' nu este valida.\n", cmdPtr);
                }
                
                bufPos = 0;
                break;
            }
        } else {
            buffer[bufPos++] = c;
        }
    }
}