#include "panic.h"
#include "kprint.h"

#include <stdbool.h>

static char const *reason_strings[] = {
    "general panic",
    "unhandled fault",
};

noreturn void panic(enum panic_reason reason)
{
    // TODO: kprintf() and friends
    kputs("kernel panic: ");
    kputs(reason_strings[reason]);
    kputs("\n");
    halt();
}

noreturn void halt(void)
{
    __asm__ ("cli\n\t");
    while (true) __asm__ ("hlt\n\t");
}
