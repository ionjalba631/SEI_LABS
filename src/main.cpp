#include <Arduino.h>
#include <stdio.h>

#ifdef ARDUINO_ARCH_AVR
#include <avr/io.h>
#endif

#include "ctrlMotor.h"

namespace {

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

}  // namespace

void setup() {
    Serial.begin(115200);

#ifdef ARDUINO_ARCH_AVR
    fdev_setup_stream(&serial_stdout, serial_putchar, nullptr, _FDEV_SETUP_WRITE);
    stdout = &serial_stdout;
#endif

    ctrlMotorInit();
}

void loop() {
    ctrlMotorUpdate();
    delay(20);
}
