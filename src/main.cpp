#include <Arduino.h>
#include "ctrlStdio.h"
#include "ctrlAccess.h"

/**
 * Punctul de intrare in aplicatie.
 * Structura modulara separa hardware-ul (dd), serviciile (ctrlStdio) 
 * si logica de business (ctrlAccess).
 */

void setup() {
    // 1. Initializam puntea STDIO (redirectioneaza printf/scanf catre LCD/Keypad)
    // De asemenea, porneste si Serial Monitor la 9600 baud pentru debug.
    ctrlStdioInit();

    // 2. Initializam modulul de securitate (configureaza LED-urile si afiseaza mesajul de start)
    ctrl_access_init();
    
    // Mesaj suplimentar de confirmare in consola de debug (Serial)
    printf("\n[Sistem pornit]\n");
}

void loop() {
    // 3. Verificam continuu daca s-a introdus un cod la Keypad.
    // Aceasta functie foloseste scanf(), care va bloca executia pana la 
    // introducerea codului urmat de tasta de confirmare (ex: '#').
    ctrl_access_update();
}