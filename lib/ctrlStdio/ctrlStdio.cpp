#include "ctrlStdio.h"

static int uartPutchar(char c, FILE *f) {
    if (c == '\n') Serial.write('\r');
    return Serial.write(c);
}

static int uartGetchar(FILE *f) {
    if (!Serial.available()) {
        return _FDEV_EOF;
    }
    return Serial.read();
}

static FILE uart_io;

void ctrlStdioInit(unsigned long baud) {
    Serial.begin(baud);
    fdev_setup_stream(&uart_io, uartPutchar, uartGetchar, _FDEV_SETUP_RW);
    stdout = stdin = &uart_io;
}