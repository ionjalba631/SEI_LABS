#include <Arduino.h>
#include "ctrlStdio.h"
#include "ctrlLed.h"

void setup() {
    // Initializam STDIO (printf/fgets) la 9600 baud
    ctrlStdioInit(UART_BAUD_RATE);
    
    // Initializam sistemul de control pentru LED
    ctrlLedInit();
}

void loop() {
    // Ascultam continuu terminalul pentru comenzi noi
    ctrlLedUpdate();
}