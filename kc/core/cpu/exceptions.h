#pragma once

#define DIVIDE_ZER0_EXCEPTION 0x0
#define DEBUG_EXCEPTION 0x1
#define NONMASKABLE_EXCEPTION 0x2
#define BREAKPOINT_EXCEPTION 0x3
#define OVERFLOW_EXCEPTION 0x4
#define BOUND_RANGE_EXCEPTION 0x5
#define INVALID_OPCODE_EXCEPTION 0x6
#define DEVICE_NOT_AVAILABLE_EXCEPTION 0x7
#define DOUBLE_FAULT_EXCEPTION 0x8
#define INVALID_TSS_EXCEPTION 0x10
#define SEGMENT_NOT_PRESENT_EXCEPTION 0x11
#define STACK_EXCEPTION 0xc
#define GENERAL_PROTECTION_EXCEPTION 0xd
#define PAGE_FAULT_EXCEPTION 0xe
#define X87_FLOAT_EXCEPTION 0x10
#define ALIGNMENT_CHECK_EXCEPTION 0x11
#define MACHINE_CHECK_EXCEPTION 0x12
#define SIMD_FLOAT_EXCEPTION 0x13

#ifndef __ASSEMBLER__

#include "kprint.h"

struct isr_context_param_regs
{
    uint64_t r9, r8, rcx, rdx, rsi, rdi;
};

struct isr_context_caller_regs
{
    uint64_t r11, r10, rax;
};

struct isr_context_callee_regs
{
    uint64_t r15, r14, r13, r12, rbp, rbx;
};

struct isr_context_entry_state
{
    uint64_t vector, code, rip, cs, rflags;
};

struct isr_context
{
    struct isr_context_callee_regs callee_regs;
    struct isr_context_caller_regs caller_regs;
    struct isr_context_param_regs param_regs;
    struct isr_context_entry_state entry_state;
};

static inline void print_exception_context(struct isr_context *context)
{
    kprintf(
            "exception %#hhx:%x context:\n"
            "interrupt entry state:\n"
            "rip: %#hx:%p rflags: %#0.16lx\n"
            "parameter registers:\n" 
            "rdi: %#0.16lx rsi: %#0.16lx rdx: %#0.16lx\n"
            "rcx: %#0.16lx r8: %#0.16lx r9: %#0.16lx\n"
            "caller registers:\n"
            "rax: %#0.16lx r10: %#0.16lx r11: %#0.16lx\n"
            "callee registers:\n"
            "rbx: %#0.16lx rbp: %#0.16lx r12: %#0.16lx\n"
            "r13: %#0.16lx r14: %#0.16lx r15: %#0.16lx\n",
            context->entry_state.vector, context->entry_state.code,
            context->entry_state.cs, context->entry_state.rip,
            context->entry_state.rflags,

            context->param_regs.rdi, context->param_regs.rsi,
            context->param_regs.rdx, context->param_regs.rcx,
            context->param_regs.r8,context->param_regs.r9,

            context->caller_regs.rax, context->caller_regs.r10,
            context->caller_regs.r11,

            context->callee_regs.rbx, context->callee_regs.rbp,
            context->callee_regs.r12, context->callee_regs.r13, 
            context->callee_regs.r14, context->callee_regs.r15);
}

void general_protection_isr(void);
void general_protection_handler(struct isr_context *context);
void page_fault_isr(void);
void page_fault_handler(struct isr_context *context);

void exceptions_init(void);

#else

.macro EXCEPTION_DECLARE name:req, vec:req
.hidden \name\()_isr
.global \name\()_isr
.extern \name\()_handler
.type \name\()_isr, @function
.endm

#endif

