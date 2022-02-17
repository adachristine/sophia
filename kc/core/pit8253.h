#pragma once
#include "timer.h"

#include <stdint.h>

void pit8253_init(void);
void pit8253_start(void);
void pit8253_stop(void);

void pit8253_set_frequency(uint32_t frequency);
uint32_t pit8253_get_frequency(void);

uint64_t pit8253_nanoseconds_elapsed(void);
uint64_t pit8253_nanoseconds_delta(void);

extern const struct timer_source pit8253_timer_source;

