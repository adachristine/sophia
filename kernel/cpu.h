#pragma once

void cpu_init(void);

void isr_install(int vec, void *isr, int ist);
void ist_install(int ist, void *stack);

void int_enable(void);
void int_disable(void);
