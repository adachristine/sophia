#include "timer.h"
#include "pit8253.h"

uint64_t timer_ticks(void)
{
    return pit8253_count();
}

