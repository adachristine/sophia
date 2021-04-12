#pragma once

#include <stdnoreturn.h>

enum panic_reason
{
    GENERAL_PANIC,
    UNHANDLED_FAULT,
};

noreturn void panic(enum panic_reason reason);
noreturn void halt(void);
