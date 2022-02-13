#pragma once

#include <stdint.h>

void pit8253_init(void);
void pit8253_start(void);
void pit8253_stop(void);
void pit8253_set_hz(uint16_t hz);

