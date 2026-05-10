#include "wokwi-api.h"

#include <stdint.h>
#include <stdlib.h>

typedef struct {
  pin_t pin_out1;
  pin_t pin_out2;
  pin_t pin_out3;
  pin_t pin_out4;
  pin_t pin_in1;
  pin_t pin_in2;
  pin_t pin_in3;
  pin_t pin_in4;
  pin_t pin_ena;
  pin_t pin_enb;
  uint32_t vs_attr;
} chip_state_t;

static void chip_pin_change(void *user_data, pin_t pin, uint32_t value);

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  chip->pin_ena = pin_init("EN A", INPUT);
  chip->pin_enb = pin_init("EN B", INPUT);
  chip->pin_in1 = pin_init("IN1", INPUT);
  chip->pin_in2 = pin_init("IN2", INPUT);
  chip->pin_in3 = pin_init("IN3", INPUT);
  chip->pin_in4 = pin_init("IN4", INPUT);
  chip->pin_out1 = pin_init("OUT1", ANALOG);
  chip->pin_out2 = pin_init("OUT2", ANALOG);
  chip->pin_out3 = pin_init("OUT3", ANALOG);
  chip->pin_out4 = pin_init("OUT4", ANALOG);
  chip->vs_attr = attr_init_float("Vs", 12.0f);

  const pin_watch_config_t watch_config = {
    .edge = BOTH,
    .pin_change = chip_pin_change,
    .user_data = chip
  };

  pin_watch(chip->pin_ena, &watch_config);
  pin_watch(chip->pin_enb, &watch_config);
  pin_watch(chip->pin_in1, &watch_config);
  pin_watch(chip->pin_in2, &watch_config);
  pin_watch(chip->pin_in3, &watch_config);
  pin_watch(chip->pin_in4, &watch_config);
}

static void chip_pin_change(void *user_data, pin_t pin, uint32_t value) {
  (void)pin;
  (void)value;

  chip_state_t *chip = (chip_state_t *)user_data;
  const int ena = pin_read(chip->pin_ena);
  const int enb = pin_read(chip->pin_enb);
  const int in1 = pin_read(chip->pin_in1);
  const int in2 = pin_read(chip->pin_in2);
  const int in3 = pin_read(chip->pin_in3);
  const int in4 = pin_read(chip->pin_in4);
  const float vs = attr_read_float(chip->vs_attr);

  if (in1) {
    pin_dac_write(chip->pin_out1, ena * vs);
  } else {
    pin_write(chip->pin_out1, LOW);
  }

  if (in2) {
    pin_dac_write(chip->pin_out2, ena * vs);
  } else {
    pin_write(chip->pin_out2, LOW);
  }

  if (in3) {
    pin_dac_write(chip->pin_out3, enb * vs);
  } else {
    pin_write(chip->pin_out3, LOW);
  }

  if (in4) {
    pin_dac_write(chip->pin_out4, enb * vs);
  } else {
    pin_write(chip->pin_out4, LOW);
  }
}
