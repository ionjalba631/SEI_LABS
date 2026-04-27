#include "ctrlStdio.h"

/*
 * ctrlStdio - Simplified STDIO bridge using only Serial
 *
 * Redirects stdout to Serial (UART0) for debug output.
 * Uses avr-libc's fdev_setup_stream() to bind a callback to a
 * FILE object that becomes stdout.
 */

/* ------------------------------------------------------------------ */
/*  stdout callback - one character at a time to Serial               */
/* ------------------------------------------------------------------ */
static int serialPutchar(char c, FILE *stream) {
    (void)stream;
    if (c == '\n') {
        Serial.write('\r');
    }
    Serial.write(c);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Single FILE object for stdout                                      */
/* ------------------------------------------------------------------ */
static FILE serial_stream;

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

void ctrlStdioInit(void) {
    /* Initialise UART */
    Serial.begin(STDIO_UART_BAUD);

    /* Bind callback and redirect stdout */
    fdev_setup_stream(&serial_stream,
                      serialPutchar,
                      NULL,
                      _FDEV_SETUP_WRITE);

    stdout = &serial_stream;
}
