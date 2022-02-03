#include "cpu.h"
#include "exceptions.h"
#include "kprint.h"
#include "panic.h"
#include "msr.h"
#include "task.h"
#include "memory.h"

#include <stddef.h>
#include <stdint.h>

#define GDT_ENTRIES 16
#define IDT_ENTRIES 256

#define CODE_SUPER_SEG_INDEX 1
#define CODE_USER_SEG_INDEX 2
#define DATA_SEG_INDEX 3
#define TASK_SEG_INDEX 4

#define EFER_SCE 1

#define MSR_EFER 0xc0000080
#define MSR_STAR 0xc0000081
#define MSR_LSTAR 0xc0000082
#define MSR_CSTAR 0xc0000083
#define MSR_SFMASK 0xc0000084

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
    CODE64_SUPER_SEG,
    CODE64_USER_SEG,
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

static struct segment_descriptor gdt[GDT_ENTRIES];
static struct gate_descriptor idt[IDT_ENTRIES];
static struct task64_segment tss;

uint64_t *get_tss_rsp0(void)
{
    return &tss.rsp0;
}

static void syscall_entry(void)
{
    kputs("system call\n");
    halt();
}

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
        case CODE64_SUPER_SEG:
            gdt[index].access = 0x9a;
            gdt[index].flags_limit16 |= 0xa0;
            break;
        case CODE64_USER_SEG:
            gdt[index].access = 0xfa;
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

void set_ist(int vec, int ist_index)
{
    // non-destructively set the ist index.
    idt[vec].access |= ist_index & 0x7;
}

static void gdt_init(void)
{   
    set_gdt(CODE_SUPER_SEG_INDEX, 0, (uint64_t)-1, CODE64_SUPER_SEG);
    set_gdt(CODE_USER_SEG_INDEX, 0, (uint64_t)-1, CODE64_USER_SEG);
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
         "mov %w2, %%ss\n"
         // make a far return frame on the stack
         // [ss] @rsp+8
         // [rip] @rsp
         "pushq %1\n"
         "lea .Lflush(%%rip), %%rax\n"
         "push %%rax\n"
         // far return, now we're in the code segment
         // defined here.
         "lretq\n"
         ".Lflush:\n"
         "ltr %w3\n"
         "popf\n"
         // dependency on pre-boot GDT is now gone
         :
         : "m"(gdtr),
        "i"(CODE_SUPER_SEG_INDEX << 3),
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
    exceptions_init();

    //TODO: fix rudimentary syscall stuff.

    uint64_t lstar_bits = (uintptr_t)syscall_entry;
    msr_write(MSR_LSTAR, lstar_bits);

    union
    {
        struct
        {
            uint32_t eip32;
            uint8_t syscall_ss;
            uint8_t syscall_cs;
            uint8_t sysret_ss;
            uint8_t sysret_cs;
        }
        fields;
        uint64_t raw;
    }
    star_bits = 
    {
        {
            0,
            DATA_SEG_INDEX,
            CODE_SUPER_SEG_INDEX,
            DATA_SEG_INDEX,
            CODE_USER_SEG_INDEX
        }
    };

    msr_write(MSR_STAR, star_bits.raw);

    uint64_t efer_bits = msr_read(MSR_EFER);
    efer_bits |= EFER_SCE; // enable syscall extensions
    msr_write(MSR_EFER, efer_bits);
}

void exceptions_init(void)
{
    isr_install(GENERAL_PROTECTION_VECTOR, &general_protection_isr, 0, 0);
    isr_install(PAGE_FAULT_VECTOR, &page_fault_isr, 0, 0);
}

void isr_install(int vec, void (*isr)(void), int trap, unsigned ist)
{
    enum gate_descriptor_type type = INT64_GATE;

    if (trap)
    {
        type = TRAP64_GATE;
    }

    set_idt(vec, CODE_SUPER_SEG_INDEX << 3, (uintptr_t)isr, type);
    // an IST greater than 7 is invalid 
    if (ist && ist <= 7)
    {
        set_ist(vec, ist);
    }
    else if (ist && ist > 7)
    {
        panic(GENERAL_PANIC);
    }
}

void ist_install(int ist, void *stack)
{
    tss.ist[ist] = (uintptr_t)stack;
}

void int_enable(void)
{
    __asm__ ("sti");
}

void int_disable(void)
{
    __asm__ ("cli");
}

