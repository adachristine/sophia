#include "cpu.h"

#include <stdint.h>

#define KERNEL_CODE_DESC 0x00af9a000000ffff
#define KERNEL_DATA_DESC 0x00cf92000000ffff 

#define KERNEL_CODE_SEL 0x8
#define KERNEL_DATA_SEL 0x10

struct gdtr64
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static uint64_t gdt[16];

void cpu_init(void)
{
    gdt[0] = 0;
    gdt[1] = KERNEL_CODE_DESC;
    gdt[2] = KERNEL_DATA_DESC;

    struct gdtr64 gdtr = {sizeof(gdt) - 1, (uint64_t)&gdt};

    __asm__ volatile
    (   "pushf\n"
        "cli\n"
        "lgdt %0\n"
        "mov %w2, %%ds\n"
        "mov %w2, %%es\n"
        "mov %w2, %%fs\n"
        "mov %w2, %%gs\n"
        "pushq %1\n"
        "lea .flush(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        ".flush:\n"
        "popf\n"
        :: "m"(gdtr), "i"(KERNEL_CODE_SEL), "r"(KERNEL_DATA_SEL)
    );
}

