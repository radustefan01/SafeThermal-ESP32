// dual-opamp.chip.c -- Wokwi dual op-amp
// https://wokwi.com/projects/409320061010147329

// Wokwi Custom Chip - For information and examples see:
// https://docs.wokwi.com/chips-api/
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2024 David Forresst

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  pin_t pin_outA;
  pin_t pin_inAP;
  pin_t pin_inAN;
  pin_t pin_outB;
  pin_t pin_inBP;
  pin_t pin_inBN;
  uint32_t gainA_attr;
  uint32_t offsetA_attr;
  uint32_t gainB_attr;
  uint32_t offsetB_attr;
} chip_state_t;

static void chip_timer_event(void *user_data);

void chip_init(void) {
  chip_state_t *chip = malloc(sizeof(chip_state_t));
  chip->pin_outA = pin_init("OUTA", ANALOG);
  chip->pin_inAP = pin_init("INA+", ANALOG);
  chip->pin_inAN = pin_init("INA-", ANALOG);
  chip->pin_outB = pin_init("OUTB", ANALOG);
  chip->pin_inBP = pin_init("INB+", ANALOG);
  chip->pin_inBN = pin_init("INB-", ANALOG);
  chip->gainA_attr = attr_init_float("gainA", 1.0);
  chip->offsetA_attr = attr_init_float("offsetA", 2.5);
  chip->gainB_attr = attr_init_float("gainB", 1.0);
  chip->offsetB_attr = attr_init_float("offsetB", 2.5);

  const timer_config_t timer_config = {
    .callback = chip_timer_event,
    .user_data = chip,
  };
  timer_t timer_id = timer_init(&timer_config);
  timer_start(timer_id, 1000, true);
}

void chip_timer_event(void *user_data) {
  chip_state_t *chip = (chip_state_t*)user_data;

  float valAP = pin_adc_read(chip->pin_inAP);
  float valAN = pin_adc_read(chip->pin_inAN);
  float gainA = attr_read_float(chip->gainA_attr);
  float VoffsetA = attr_read_float(chip->offsetA_attr);
  float valBP = pin_adc_read(chip->pin_inBP);
  float valBN = pin_adc_read(chip->pin_inBN);
  float gainB = attr_read_float(chip->gainB_attr);
  float VoffsetB = attr_read_float(chip->offsetB_attr);

  float calcA = (valAP - valAN)*gainA+VoffsetA;
  float calcB = (valBP - valBN)*gainB+VoffsetB;

  // note that the writes are constrained rail-to-rail 0-5V/0-1023 by the API
  pin_dac_write(chip->pin_outA, calcA);
  pin_dac_write(chip->pin_outB, calcB);
}
