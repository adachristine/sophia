#include "irq.h"

#include <stdatomic.h>

static volatile atomic_uint_fast64_t irq_lock_count = 0;

uint64_t irq_lock(void)
{
    uint64_t flags;

    __asm__ volatile 
        ( 
         "pushf\n"
         "pop %0\n"
         "cli\n"
         : 
         "=r"(flags)
        );

    irq_lock_count++;

    return flags;
}

void irq_unlock(uint64_t flags)
{
    if (atomic_load(&irq_lock_count) >= 1)
    {
        irq_lock_count--;
    }

    if (atomic_load(&irq_lock_count) == 0)
    {
        __asm__ volatile
            (
             "push %q0\n"
             "popf"
             :
             :
             "r"(flags)
            );
    }
}

