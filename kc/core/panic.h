#pragma once

#include <stdnoreturn.h>

#define PANIC(reason) panic(__FILE__, __LINE__, reason)

enum panic_reason
{
    GENERAL_PANIC,
    UNHANDLED_FAULT,
    OUT_OF_MEMORY,
    DEAD_END
};

noreturn void panic(const char *file, int line, enum panic_reason reason);
noreturn void halt(void);

