#ifndef DD_BUTTON_H
#define DD_BUTTON_H

#include <Arduino.h>

/*
 * ddButton – Hardware driver pentru buton fizic cu debounce software
 *
 * Detectează apăsarea unui buton cu debounce și edge detection.
 * Suportă buton activ LOW (pull-up intern activat).
 *
 * Wiring (Arduino Mega 2560):
 *   Buton -> Pin definit la init, cealaltă parte la GND
 *   Pull-up intern activat automat
 */

#define DD_BUTTON_DEBOUNCE_MS 20  /* Timp debounce în ms */

typedef enum {
    DD_BTN_RELEASED = 0,
    DD_BTN_PRESSED  = 1
} DdButtonState;

typedef struct DdButton {
    uint8_t       pinNumber;
    DdButtonState currentState;
    DdButtonState lastRawState;
    uint32_t      lastDebounceTime;
    uint8_t       pressedEdge;   /* 1 dacă tocmai s-a detectat apăsare (rising edge) */
} DdButton;

void          ddButtonInit(DdButton *btn, uint8_t pin);
void          ddButtonUpdate(DdButton *btn);          /* Apelat periodic (ex: 10 ms) */
DdButtonState ddButtonGetState(DdButton *btn);
uint8_t       ddButtonWasPressed(DdButton *btn);      /* Returnează 1 o singură dată per apăsare */

#endif /* DD_BUTTON_H */