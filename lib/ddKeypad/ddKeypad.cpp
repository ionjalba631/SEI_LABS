#include "ddKeypad.h"

/*
 * ddKeypad – 4x4 matrix keypad driver
 * Rows are outputs (driven LOW one at a time).
 * Columns are inputs with internal pull-ups.
 *
 * Arduino Mega pin → AVR port/bit mapping used here:
 *
 *  Row pins (OUTPUT):
 *    Pin 22 → PA0
 *    Pin 23 → PA1
 *    Pin 24 → PA2
 *    Pin 26 → PA4
 *
 *  Col pins (INPUT, pull-up):
 *    Pin 28 → PA6
 *    Pin 30 → PC7
 *    Pin 32 → PC5
 *    Pin 34 → PC3
 */

/* Number of rows / columns */
#define ROWS 4
#define COLS 4

/* Key map matching physical layout */
static const char keyMap[ROWS][COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

/* ------------------------------------------------------------------ */
/*  Row pin definitions (output)                                        */
/* ------------------------------------------------------------------ */
/* Each row: DDR register, PORT register, PIN register, bit index */
typedef struct {
    volatile uint8_t *ddr;
    volatile uint8_t *port;
    volatile uint8_t *pin_reg;
    uint8_t           bit;
} GpioPin;

static const GpioPin rowPins[ROWS] = {
    {&DDRA, &PORTA, &PINA, 0},   /* Pin 22 – PA0 */
    {&DDRA, &PORTA, &PINA, 1},   /* Pin 23 – PA1 */
    {&DDRA, &PORTA, &PINA, 2},   /* Pin 24 – PA2 */
    {&DDRA, &PORTA, &PINA, 4}    /* Pin 26 – PA4 */
};

/* ------------------------------------------------------------------ */
/*  Column pin definitions (input with pull-up)                         */
/* ------------------------------------------------------------------ */
static const GpioPin colPins[COLS] = {
    {&DDRA, &PORTA, &PINA, 6},   /* Pin 28 – PA6 */
    {&DDRC, &PORTC, &PINC, 7},   /* Pin 30 – PC7 */
    {&DDRC, &PORTC, &PINC, 5},   /* Pin 32 – PC5 */
    {&DDRC, &PORTC, &PINC, 3}    /* Pin 34 – PC3 */
};

/* ------------------------------------------------------------------ */
/*  Internal helpers                                                     */
/* ------------------------------------------------------------------ */
static void rowHigh(uint8_t r) {
    *rowPins[r].port |=  (1 << rowPins[r].bit);
}
static void rowLow(uint8_t r) {
    *rowPins[r].port &= ~(1 << rowPins[r].bit);
}
static uint8_t colRead(uint8_t c) {
    return (*colPins[c].pin_reg >> colPins[c].bit) & 0x01;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                           */
/* ------------------------------------------------------------------ */

void ddKeypadInit(void) {
    /* Rows: OUTPUT, initially HIGH (inactive) */
    for (uint8_t r = 0; r < ROWS; r++) {
        *rowPins[r].ddr  |=  (1 << rowPins[r].bit);
        *rowPins[r].port |=  (1 << rowPins[r].bit);
    }

    /* Columns: INPUT with internal pull-up */
    for (uint8_t c = 0; c < COLS; c++) {
        *colPins[c].ddr  &= ~(1 << colPins[c].bit);  /* INPUT  */
        *colPins[c].port |=  (1 << colPins[c].bit);  /* PULLUP */
    }
}

/*
 * Scan all rows/columns once.
 * Returns the key character if exactly one key is pressed, else DD_KP_NO_KEY.
 */
char ddKeypadGetKey(void) {
    for (uint8_t r = 0; r < ROWS; r++) {
        rowLow(r);                         /* Drive this row LOW      */
        delayMicroseconds(10);             /* Settle time             */

        for (uint8_t c = 0; c < COLS; c++) {
            if (colRead(c) == 0) {         /* Column pulled LOW → key pressed */
                rowHigh(r);
                return keyMap[r][c];
            }
        }

        rowHigh(r);                        /* Restore row HIGH        */
    }
    return DD_KP_NO_KEY;
}

/*
 * Block until a key is pressed AND released.
 * Simple debounce: 20 ms stable press then wait for release.
 */
char ddKeypadWaitKey(void) {
    char key;

    /* Wait for a key press */
    do {
        key = ddKeypadGetKey();
    } while (key == DD_KP_NO_KEY);

    delay(20); /* Debounce */

    /* Wait for release */
    while (ddKeypadGetKey() != DD_KP_NO_KEY) {
        delay(5);
    }

    return key;
}