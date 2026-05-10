#include "keypad_dd.h"

#include "ddKeypad.h"

void keypad_init() {
    ddKeypadInit();
}

char keypad_get_key() {
    return ddKeypadGetKey();
}
