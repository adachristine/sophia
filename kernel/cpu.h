#pragma once

void cpu_init(void);
void isr_install(int vec, void *isr, int ist);

