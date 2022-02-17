#pragma once
#include <stdint.h>

#define TIMER_MILLISECOND 1000ULL
#define TIMER_MICROSECOND 1000000ULL
#define TIMER_NANOSECOND 1000000000ULL

struct timer_source
{
    const char *name;
    void (*init)(void);
    void (*start)(void);
    void (*stop)(void);

    void (*set_frequency)(uint32_t frequency);
    uint32_t (*get_frequency)(void);

    uint64_t (*nanoseconds_elapsed)(void);
    uint64_t (*nanoseconds_delta)(void);
};

