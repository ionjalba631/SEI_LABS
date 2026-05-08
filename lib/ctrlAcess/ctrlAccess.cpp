#include "ctrlAccess.h"
#include "ddLed.h"
#include "ctrlStdio.h"
#include "ddKeypad.h"

static DdLed led_ok, led_fail;

/* Buffer pentru codul introdus */
static char input_code[9];  /* Max 8 caractere + null */
static uint8_t code_len = 0;

/* Flag pentru a urmări dacă o tastă a fost procesată și eliberată */
static uint8_t key_processed = 0;

void ctrl_access_init(void) {
    ddLedInit(&led_ok, LED_GREEN_PIN);
    ddLedInit(&led_fail, LED_RED_PIN);
    
    /* Afișăm mesajul pe rândul 0 */
    ctrlStdioSetWrap(0);  /* Disable auto wrap */
    ctrlStdioSetRow(0);
    ctrlStdioSetCol(0);
    printf("Introduceti cod:");
    
    /* Pregătim rândul 1 pentru introducere cod */
    ctrlStdioSetRow(1);
    ctrlStdioSetCol(0);
    
    /* Reset buffer cod */
    input_code[0] = '\0';
    code_len = 0;
    key_processed = 0;
}

void ctrl_access_update(void) {
    /* Citire non-blocking de la keypad */
    char key = ddKeypadGetKey();
    
    /* Dacă nicio tastă apăsată, resetăm flag-ul */
    if (key == DD_KP_NO_KEY) {
        key_processed = 0;
        return;
    }
    
    /* Dacă o tastă e apăsată dar deja procesată, ignorăm */
    if (key_processed) {
        return;
    }
    
    /* Marcăm tasta ca procesată până la eliberare */
    key_processed = 1;
    
    /* Verificăm dacă e tasta # (confirmare cod) */
    if (key == '#') {
        /* Verificăm codul introdus */
        if (code_len > 0 && strcmp(input_code, SECRET_CODE) == 0) {
            /* Cod valid */
            ctrlStdioSetRow(1);
            ctrlStdioSetCol(0);
            ctrlStdioClearRow(1);
            ctrlStdioSetRow(1);
            ctrlStdioSetCol(0);
            printf("Cod Valid!");
            ddLedOn(&led_ok);
            ddLedOff(&led_fail);
        } else {
            /* Cod invalid */
            ctrlStdioSetRow(1);
            ctrlStdioSetCol(0);
            ctrlStdioClearRow(1);
            ctrlStdioSetRow(1);
            ctrlStdioSetCol(0);
            printf("Cod Invalid!");
            ddLedOff(&led_ok);
            ddLedOn(&led_fail);
        }
        
        delay(2000);
        
        /* Reset display */
        ctrlStdioSetRow(0);
        ctrlStdioSetCol(0);
        ctrlStdioClearRow(0);
        ctrlStdioSetRow(0);
        ctrlStdioSetCol(0);
        printf("Introduceti cod:");
        
        ctrlStdioSetRow(1);
        ctrlStdioSetCol(0);
        ctrlStdioClearRow(1);
        ctrlStdioSetRow(1);
        ctrlStdioSetCol(0);
        
        /* Reset buffer */
        input_code[0] = '\0';
        code_len = 0;
    }
    /* Verificăm dacă e tasta * (ștergere cod) */
    else if (key == '*') {
        input_code[0] = '\0';
        code_len = 0;
        ctrlStdioSetRow(1);
        ctrlStdioSetCol(0);
        ctrlStdioClearRow(1);
        ctrlStdioSetRow(1);
        ctrlStdioSetCol(0);
    }
    /* Adăugăm caracter la cod (cifre) */
    else if (code_len < 8 && (key >= '0' && key <= '9')) {
        input_code[code_len++] = key;
        input_code[code_len] = '\0';
        
        /* Afișăm caracterul pe rândul 1 */
        ctrlStdioSetRow(1);
        printf("%c", key);
    }
    /* Alte taste - ignorăm */
}