#pragma once

#include <stdnoreturn.h>

enum panic_reason
{
    GENERAL_PANIC,
    UNHANDLED_FAULT,
    OUT_OF_MEMORY,
};

noreturn void panic(enum panic_reason reason);
noreturn void halt(void);
