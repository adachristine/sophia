#pragma once

#include "task.h"

#include <stdnoreturn.h>

void cpu_init(void);

noreturn void cpu_task_begin(void);
void cpu_task_set(struct kc_thread *task);

void isr_install(int vec, void (*isr)(void), int ist, int trap);
void ist_install(int ist, void *stack);

void int_enable(void);
void int_disable(void);

