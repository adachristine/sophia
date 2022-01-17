#include "msr.h"

uint64_t msr_read(uint32_t index)
{
    uint32_t low;
    uint32_t high;

    __asm__ volatile
        (
         "rdmsr\n\t"
         : "=a"(low), "=d"(high)
         : "c"(index)
        );

    return ((uint64_t)high << 32)|low;
}

void msr_write(uint32_t index, uint64_t value)
{
    uint32_t low = value & 0xffffffff;
    uint32_t high = value >> 32;

    __asm__ volatile
        (
         "wrmsr\n\t"
         :: "a"(low), "d"(high), "c"(index)
        );
}

