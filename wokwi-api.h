#ifndef WOKWI_API_H
#define WOKWI_API_H

#include <stdint.h>

typedef int pin_t;

typedef enum {
  INPUT = 0,
  OUTPUT = 1,
  ANALOG = 2
} pin_mode_t;

typedef enum {
  RISING = 1,
  FALLING = 2,
  BOTH = 3
} pin_edge_t;

typedef struct {
  pin_edge_t edge;
  void (*pin_change)(void *user_data, pin_t pin, uint32_t value);
  void *user_data;
} pin_watch_config_t;

#ifndef LOW
#define LOW 0
#endif

#ifndef HIGH
#define HIGH 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

pin_t pin_init(const char *name, pin_mode_t mode);
void pin_watch(pin_t pin, const pin_watch_config_t *config);
int pin_read(pin_t pin);
void pin_write(pin_t pin, int value);
void pin_dac_write(pin_t pin, float value);
uint32_t attr_init_float(const char *name, float initial_value);
float attr_read_float(uint32_t attr);

#ifdef __cplusplus
}
#endif

#endif
