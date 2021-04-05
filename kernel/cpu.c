#include "cpu.h"

#include <stdint.h>

#define GDT_ENTRIES 16
#define IDT_ENTRIES 256

#define CODE_SEG_INDEX 1
#define DATA_SEG_INDEX 2
#define TASK_SEG_INDEX 3

struct dtr64
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct segment_descriptor
{
    uint16_t limit0;
    uint16_t base0;
    uint8_t base16;
    uint8_t access;
    uint8_t flags_limit16;
    uint8_t base24;
} __attribute__((packed));

enum segment_descriptor_type
{
    CODE64_SEG,
    DATA_SEG,
    TASK64_SEG,
};

struct gate_descriptor
{
    uint16_t offset0;
    uint16_t selector;
    uint8_t ist;
    uint8_t access;
    uint16_t offset16;
    uint64_t offset32;
} __attribute__((packed));

enum gate_descriptor_type
{
    INT64_GATE,
    TRAP64_GATE,
};

struct task64_segment
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint16_t reserved2;
    uint16_t iomap_base;
};

// TODO: is 16 enough?
static struct segment_descriptor gdt[GDT_ENTRIES];
static struct gate_descriptor idt[IDT_ENTRIES];
static struct task64_segment tss;

static void set_gdt(int index,
                    uint64_t base,
                    uint64_t limit,
                    enum segment_descriptor_type type)
{
    gdt[index].limit0 = limit & 0xffff;
    gdt[index].base0 = base & 0xffff;
    gdt[index].base16 = (base >> 16) & 0xff;
    gdt[index].flags_limit16 = (limit >> 16) & 0xf;
    gdt[index].base24 = (base >> 24) & 0xff;
    
    switch (type)
    {
        case CODE64_SEG:
            gdt[index].access = 0x9a;
            gdt[index].flags_limit16 |= 0xa0;
            break;
        case DATA_SEG:
            gdt[index].access = 0x92;
            gdt[index].flags_limit16 |= 0xc0;
            break;
        case TASK64_SEG:
            gdt[index].access = 0x89;
            //TODO: make this cleaner i don't like it
            gdt[index+1].limit0 = (base >> 32) & 0xffff;
            gdt[index+1].base0 = (base >> 48) & 0xffff;
            break;
    };
}

static void set_idt(int vec,
                    uint16_t selector,
                    uint64_t offset,
                    enum gate_descriptor_type type)
{
    idt[vec].offset0 = offset & 0xffff;
    idt[vec].selector = selector;
    idt[vec].offset16 = (offset >> 16) & 0xffff;
    idt[vec].offset32 = (offset >> 32) & 0xffffffff;

    switch (type)
    {
        case INT64_GATE:
            idt[vec].access = 0x8e;
            break;
        case TRAP64_GATE:
            idt[vec].access = 0x8f;
            break;
    }
}

static void gdt_init(void)
{   
    set_gdt(CODE_SEG_INDEX, 0, (uint64_t)-1, CODE64_SEG);
    set_gdt(DATA_SEG_INDEX, 0, (uint64_t)-1, DATA_SEG);
    set_gdt(TASK_SEG_INDEX, (uint64_t)&tss, sizeof(tss) - 1, TASK64_SEG);

    struct dtr64 gdtr = {sizeof(gdt) - 1, (uint64_t)&gdt};

    __asm__ volatile
    (   
        // can't have interrupts during GDT reload
        // but don't re-enable interrupts if they were already disabled
        "pushf\n"
        "cli\n"
        "lgdt %0\n"
        "mov %w2, %%ds\n"
        "mov %w2, %%es\n"
        "mov %w2, %%fs\n"
        "mov %w2, %%gs\n"
        // make a far return frame on the stack
        // [ss] @rsp+8
        // [rip] @rsp
        "pushq %1\n"
        "lea .flush(%%rip), %%rax\n"
        "push %%rax\n"
        // far return, now we're in the code segment
        // defined here.
        "lretq\n"
        ".flush:\n"
        "ltr %w3\n"
        "popf\n"
        // dependency on pre-boot GDT is now gone
        :
        : "m"(gdtr),
          "i"(CODE_SEG_INDEX << 3),
          "r"(DATA_SEG_INDEX << 3),
          "r"(TASK_SEG_INDEX << 3)
    );

    
}

static void idt_init(void)
{
    struct dtr64 idtr = {sizeof(idt) - 1, (uint64_t)&idt};
    
    __asm__ volatile
    (
        // can't have interrupts enabled during IDT reload--obviously.
        // as with GDT don't re-enable interrupts if they were already
        // disabled.
        "pushf\n"
        "cli\n"
        "lidt %0\n"
        "popf\n"
        // much more straightforward than the gdt code.
        :: "m"(idtr)
    );
}

void cpu_init(void)
{
    gdt_init();
    idt_init();
}

