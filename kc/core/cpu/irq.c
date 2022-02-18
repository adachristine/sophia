#include "irq.h"

#include <stdatomic.h>

static volatile atomic_uint_fast64_t irq_lock_count = 0;

void irq_lock(void)
{
    irq_lock_count++;
    __asm__ volatile ("cli;");
}

void irq_unlock(void)
{
    if (atomic_load(&irq_lock_count) >= 1)
    {
        irq_lock_count--;
    }

    if (atomic_load(&irq_lock_count) == 0)
    {
        __asm__ volatile ("sti;");
    }
}

