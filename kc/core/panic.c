#include "panic.h"
#include "kprint.h"

#include <stdbool.h>

static char const *reason_strings[] = {
    "general panic",
    "unhandled fault",
    "out of memory"
};

noreturn void panic(enum panic_reason reason)
{
    // TODO: kprintf() and friends
    if (reason > sizeof(reason_strings) / sizeof(*reason_strings))
    {
        reason = 0;
    }
    kprintf("kernel panic: %s\n", reason_strings[reason]);
    halt();
}

noreturn void halt(void)
{
    __asm__ ("cli\n\t");
    while (true) __asm__ ("hlt\n\t");
}
