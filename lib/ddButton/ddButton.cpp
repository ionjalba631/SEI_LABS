#include "ddButton.h"

/*
 * ddButton – Driver buton cu debounce software
 *
 * Algoritmul de debounce:
 *   - Citim starea brută a pinului la fiecare apel al ddButtonUpdate()
 *   - Dacă starea brută diferă de ultima stare validă, pornim un timer
 *   - Dacă starea rămâne stabilă mai mult de DD_BUTTON_DEBOUNCE_MS,
 *     actualizăm starea confirmată
 *   - pressedEdge este setat la 1 exact un ciclu (la prima confirmare a apăsării)
 */

void ddButtonInit(DdButton *btn, uint8_t pin) {
    if (btn == NULL) return;

    btn->pinNumber        = pin;
    btn->currentState     = DD_BTN_RELEASED;
    btn->lastRawState     = DD_BTN_RELEASED;
    btn->lastDebounceTime = 0;
    btn->pressedEdge      = 0;

    /* Activăm pull-up intern → buton activ LOW */
    pinMode(btn->pinNumber, INPUT_PULLUP);
}

void ddButtonUpdate(DdButton *btn) {
    if (btn == NULL) return;

    /* Citim starea brută: buton activ LOW → inversat pentru logică pozitivă */
    DdButtonState rawState = (digitalRead(btn->pinNumber) == LOW)
                             ? DD_BTN_PRESSED
                             : DD_BTN_RELEASED;

    /* Resetăm edge-ul la fiecare ciclu (one-shot) */
    btn->pressedEdge = 0;

    /* Dacă s-a schimbat starea brută, repornim debounce timer */
    if (rawState != btn->lastRawState) {
        btn->lastDebounceTime = millis();
        btn->lastRawState     = rawState;
    }

    /* Dacă starea e stabilă mai mult de pragul de debounce */
    if ((millis() - btn->lastDebounceTime) >= DD_BUTTON_DEBOUNCE_MS) {
        if (rawState != btn->currentState) {
            /* Detectăm rising edge (RELEASED → PRESSED) */
            if (rawState == DD_BTN_PRESSED && btn->currentState == DD_BTN_RELEASED) {
                btn->pressedEdge = 1;
            }
            btn->currentState = rawState;
        }
    }
}

DdButtonState ddButtonGetState(DdButton *btn) {
    if (btn == NULL) return DD_BTN_RELEASED;
    return btn->currentState;
}

uint8_t ddButtonWasPressed(DdButton *btn) {
    if (btn == NULL) return 0;
    return btn->pressedEdge;
}