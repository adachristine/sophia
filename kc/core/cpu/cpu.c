#include "cpu.h"
#include "exceptions.h"
#include "panic.h"
#include "msr.h"
#include "task.h"
#include "memory.h"
#include "descriptor.h"
#include "irq.h"

#include <stddef.h>
#include <stdint.h>

#include <lib/kstdio.h>

#define GDT_ENTRIES 16
#define IDT_ENTRIES 256

enum kernel_segment_indices
{
    NULL_SEG,
    CODE64_SUPER_SEG_INDEX,
    DATA_SUPER_SEG_INDEX,
    CODE32_USER_SEG_INDEX,
    DATA32_USER_SEG_INDEX,
    CODE64_USER_SEG_INDEX,
    DATA64_USER_SEG_INDEX,
    TASK64_SEG_INDEX
};

#define EFER_SCE 1

#define MSR_EFER 0xc0000080
#define MSR_STAR 0xc0000081
#define MSR_LSTAR 0xc0000082
#define MSR_CSTAR 0xc0000083
#define MSR_SFMASK 0xc0000084

struct cpu_state
{
    struct segment_descriptor gdt[GDT_ENTRIES];
    struct gate_descriptor idt[IDT_ENTRIES];
    struct task64_segment tss;
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
    kprintf("system call\n");
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
        case CODE32_USER_SEG:
            gdt[index].access = 0xfa;
            gdt[index].flags_limit16 |= 0xc0;
        case DATA_SUPER_SEG:
            gdt[index].access = 0x92;
            gdt[index].flags_limit16 |= 0xc0;
            break;
        case DATA_USER_SEG:
            gdt[index].access = 0xf2;
            gdt[index].flags_limit16 |= 0xc0;
        case TASK64_SEG:
            gdt[index].access = 0x89;
            //TODO: make this cleaner i don't like it
            gdt[index+1].limit0 = (base >> 32) & 0xffff;
            gdt[index+1].base0 = (base >> 48) & 0xffff;
            break;
    };
}

static void set_idt(
        int vec,
        uint16_t selector,
        uint64_t offset,
        enum gate_descriptor_type type)
{
    // TODO: panic() on attempt to set an idte outside of bounds?
    if (vec >= IDT_ENTRIES)
    {
        return;
    }

    idt[vec].offset0 = offset & 0xffff;
    idt[vec].selector = selector;
    idt[vec].offset16 = (offset >> 16) & 0xffff;
    idt[vec].offset32 = (offset >> 32) & 0xffffffff;

    switch (type)
    {
        case EMPTY_GATE:
            idt[vec].access = 0x0;
            break;
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

static void gdt_flush(uint16_t code_seg, uint16_t data_seg)
{
    uint64_t rflags = irq_lock();
    __asm__ volatile
        (
         "mov %w1, %%ds\n"
         "mov %w1, %%es\n"
         "mov %w1, %%fs\n"
         "mov %w1, %%gs\n"
         "mov %w1, %%ss\n"
         // make a far return frame on the stack
         // [ss] @rsp+8
         // [rip] @rsp
         "push %q0\n"
         "lea .Lflush(%%rip), %%rax\n"
         "push %%rax\n"
         // far return, now we're in the provided code segment
         "lretq\n"
         ".Lflush:\n"
         :
         :
            "r"(code_seg << 3),
            "r"(data_seg << 3)
        );
    irq_unlock(rflags);
}

static void gdt_load_task(uint16_t task_seg)
{
    uint64_t rflags = irq_lock();
    __asm__ volatile
        (
         "ltr %w0\n"
         :
         :
            "r"(task_seg << 3)
        );
    irq_unlock(rflags);
}

static void gdt_load(struct dtr64 gdtr)
{
    uint64_t rflags = irq_lock();
    __asm__ volatile
        (   
         "lgdt %0\n"
         :
         : 
            "m"(gdtr)
        );
    irq_unlock(rflags);
}

static struct dtr64 gdt_init(void)
{   
    set_gdt(CODE64_SUPER_SEG_INDEX, 0, (uint64_t)-1, CODE64_SUPER_SEG);
    set_gdt(DATA_SUPER_SEG_INDEX, 0, (uint64_t)-1, DATA_SUPER_SEG);
    set_gdt(CODE32_USER_SEG_INDEX, 0, (uint64_t)-1, CODE32_USER_SEG);
    set_gdt(DATA32_USER_SEG_INDEX, 0, (uint64_t)-1, DATA_USER_SEG);
    set_gdt(CODE64_USER_SEG_INDEX, 0, (uint64_t)-1, CODE64_USER_SEG);
    set_gdt(DATA64_USER_SEG_INDEX, 0, (uint64_t)-1, DATA_USER_SEG);
    set_gdt(TASK64_SEG_INDEX, (uint64_t)&tss, sizeof(tss) - 1, TASK64_SEG);

    struct dtr64 gdtr = {sizeof(gdt) - 1, (uint64_t)&gdt};

    gdt_load(gdtr);
    gdt_flush(CODE64_SUPER_SEG_INDEX, DATA_SUPER_SEG_INDEX);
    gdt_load_task(TASK64_SEG_INDEX);

    return gdtr;
}

static struct dtr64 idt_init(void)
{
    struct dtr64 idtr = {sizeof(idt) - 1, (uint64_t)&idt};

    uint64_t rflags = irq_lock();
    __asm__ volatile
        (
         "lidt %0\n"
         :
         : 
            "m"(idtr)
        );
    irq_unlock(rflags);

    return idtr;
}

void cpu_init(void)
{
    gdt_init();
    idt_init();
    exceptions_init();

    //TODO: fix rudimentary syscall stuff.

    uintptr_t lstar_bits = (uintptr_t)syscall_entry;
    msr_write(MSR_LSTAR, lstar_bits);

    union
    {
        struct
        {
            uint32_t eip32;
            uint16_t syscall_cs;
            uint16_t sysret_cs;
        }
        fields;
        uint64_t raw;
    }
    star_bits = 
    {
        {
            0,
            CODE64_SUPER_SEG_INDEX << 3,
            CODE32_USER_SEG_INDEX << 3 | 3
        }
    };

    msr_write(MSR_STAR, star_bits.raw);

    uint64_t efer_bits = msr_read(MSR_EFER);
    efer_bits |= EFER_SCE; // enable syscall extensions
    msr_write(MSR_EFER, efer_bits);
}

void exceptions_init(void)
{
    isr_install(
            GENERAL_PROTECTION_EXCEPTION,
            &general_protection_isr,
            0,
            0);
    isr_install(PAGE_FAULT_EXCEPTION, &page_fault_isr, 0, 0);
}

void isr_install(int vec, void (*isr)(void), int trap, unsigned ist)
{
    enum gate_descriptor_type type = trap ? TRAP64_GATE : INT64_GATE;

    set_idt(vec, CODE64_SUPER_SEG_INDEX << 3, (uintptr_t)isr, type);

    if (ist && ist <= 7)
    {
        set_ist(vec, ist);
    }
    else if (ist && ist > 7)
    {
        PANIC(GENERAL_PANIC);
    }
}

void ist_install(int ist, void *stack)
{
    tss.ist[ist] = (uintptr_t)stack;
}

