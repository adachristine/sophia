#pragma once

#include <stdint.h>

void cpu_init(void);

void isr_install(int vec, void (*isr)(void), int ist, int trap);
void ist_install(int ist, void *stack);

void int_enable(void);
void int_disable(void);

uint64_t msr_read(uint32_t index);
void msr_write(uint32_t index, uint64_t value);

