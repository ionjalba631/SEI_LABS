#include "ddLcd.h"

/*
 * ddLcd – HD44780 LCD driver, 4-bit interface
 * All timing is done with delayMicroseconds() / delay().
 * No external library is used.
 *
 * Pin mapping on Arduino Mega 2560:
 *
 *  LCD Signal | Arduino Pin | AVR Port/Bit
 *  -----------|-------------|-------------
 *  RS         |    25       | PA3
 *  E          |    27       | PA5
 *  D4         |    29       | PA7
 *  D5         |    31       | PC6
 *  D6         |    33       | PC4
 *  D7         |    35       | PC2
 */

/* --- Port helpers -------------------------------------------------- */
/* RS: PA3 */
#define RS_DDR   DDRA
#define RS_PORT  PORTA
#define RS_BIT   3

/* E:  PA5 */
#define E_DDR    DDRA
#define E_PORT   PORTA
#define E_BIT    5

/* D4: PA7 */
#define D4_DDR   DDRA
#define D4_PORT  PORTA
#define D4_BIT   7

/* D5: PC6 */
#define D5_DDR   DDRC
#define D5_PORT  PORTC
#define D5_BIT   6

/* D6: PC4 */
#define D6_DDR   DDRC
#define D6_PORT  PORTC
#define D6_BIT   4

/* D7: PC2 */
#define D7_DDR   DDRC
#define D7_PORT  PORTC
#define D7_BIT   2

/* --- Bit manipulation macros --------------------------------------- */
#define PIN_HIGH(port, bit)  ((port) |=  (1 << (bit)))
#define PIN_LOW(port, bit)   ((port) &= ~(1 << (bit)))

/* --- Pulse Enable pin ---------------------------------------------- */
static void lcdPulseEnable(void) {
    PIN_HIGH(E_PORT, E_BIT);
    delayMicroseconds(1);      /* Enable pulse width >= 450 ns */
    PIN_LOW(E_PORT, E_BIT);
    delayMicroseconds(50);     /* Commands need > 37 µs to settle */
}

/* --- Write one nibble (high 4 bits of 'data' sent as D4-D7) -------- */
static void lcdWriteNibble(uint8_t nibble) {
    /* D4 <- bit 4 */
    if (nibble & 0x10) PIN_HIGH(D4_PORT, D4_BIT);
    else               PIN_LOW (D4_PORT, D4_BIT);

    /* D5 <- bit 5 */
    if (nibble & 0x20) PIN_HIGH(D5_PORT, D5_BIT);
    else               PIN_LOW (D5_PORT, D5_BIT);

    /* D6 <- bit 6 */
    if (nibble & 0x40) PIN_HIGH(D6_PORT, D6_BIT);
    else               PIN_LOW (D6_PORT, D6_BIT);

    /* D7 <- bit 7 */
    if (nibble & 0x80) PIN_HIGH(D7_PORT, D7_BIT);
    else               PIN_LOW (D7_PORT, D7_BIT);

    lcdPulseEnable();
}

/* --- Send a full byte (command or data) in 4-bit mode -------------- */
static void lcdSendByte(uint8_t rs, uint8_t data) {
    if (rs) PIN_HIGH(RS_PORT, RS_BIT);   /* RS=1 → data register    */
    else    PIN_LOW (RS_PORT, RS_BIT);   /* RS=0 → instruction reg  */

    lcdWriteNibble(data & 0xF0);         /* High nibble first        */
    lcdWriteNibble(data << 4);           /* Low nibble second        */
}

/* --- Public API ---------------------------------------------------- */

void ddLcdInit(void) {
    /* Set all LCD pins as outputs */
    RS_DDR |= (1 << RS_BIT);
    E_DDR  |= (1 << E_BIT);
    D4_DDR |= (1 << D4_BIT);
    D5_DDR |= (1 << D5_BIT);
    D6_DDR |= (1 << D6_BIT);
    D7_DDR |= (1 << D7_BIT);

    /* All pins LOW initially */
    PIN_LOW(RS_PORT, RS_BIT);
    PIN_LOW(E_PORT,  E_BIT);
    PIN_LOW(D4_PORT, D4_BIT);
    PIN_LOW(D5_PORT, D5_BIT);
    PIN_LOW(D6_PORT, D6_BIT);
    PIN_LOW(D7_PORT, D7_BIT);

    delay(50); /* Wait for LCD power-on (>40 ms) */

    /* HD44780 initialization sequence – 4-bit mode */
    PIN_LOW(RS_PORT, RS_BIT);

    /* Send 0x30 three times (8-bit mode preamble) */
    lcdWriteNibble(0x30);
    delay(5);
    lcdWriteNibble(0x30);
    delayMicroseconds(150);
    lcdWriteNibble(0x30);
    delayMicroseconds(150);

    /* Switch to 4-bit mode */
    lcdWriteNibble(0x20);
    delayMicroseconds(150);

    /* Function set: 4-bit, 2 lines, 5x8 font */
    lcdSendByte(0, 0x28);

    /* Display OFF */
    lcdSendByte(0, 0x08);

    /* Clear display */
    lcdSendByte(0, 0x01);
    delay(2); /* Clear needs >1.52 ms */

    /* Entry mode: increment cursor, no shift */
    lcdSendByte(0, 0x06);

    /* Display ON, cursor OFF, blink OFF */
    lcdSendByte(0, 0x0C);
}

void ddLcdClear(void) {
    lcdSendByte(0, 0x01);
    delay(2);
}

void ddLcdSetCursor(uint8_t col, uint8_t row) {
    /* Row 0 → DDRAM address 0x00; Row 1 → DDRAM address 0x40 */
    static const uint8_t rowOffset[2] = {0x00, 0x40};
    if (row >= DD_LCD_ROWS) row = DD_LCD_ROWS - 1;
    if (col >= DD_LCD_COLS) col = DD_LCD_COLS - 1;
    lcdSendByte(0, 0x80 | (rowOffset[row] + col));
}

void ddLcdPutChar(char c) {
    lcdSendByte(1, (uint8_t)c);
}

void ddLcdPrint(const char *str) {
    while (*str) {
        ddLcdPutChar(*str++);
    }
}