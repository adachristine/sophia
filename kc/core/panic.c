#include "panic.h"

#include <lib/kstdio.h>

#include <stdbool.h>

static char const *reason_strings[] = {
    "general panic",
    "unhandled fault",
    "out of memory",
    "dead-end routine"
};

noreturn void panic(const char *file, int line, enum panic_reason reason)
{
    // TODO: kprintf() and friends
    if (reason > sizeof(reason_strings) / sizeof(*reason_strings))
    {
        reason = 0;
    }
    kprintf(
            "kernel panic:%s:%u %s\n",
            file,
            line,
            reason_strings[reason]);
    halt();
}

noreturn void halt(void)
{
    __asm__ ("cli\n\t");
    while (true) __asm__ ("hlt\n\t");
}

