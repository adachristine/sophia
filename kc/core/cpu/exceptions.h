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

void general_protection_isr();
void general_protection_handler();
void page_fault_isr();
void page_fault_handler();

void exceptions_init(void);

#else

.macro EXCEPTION_DECLARE name:req, vec:req
.hidden \name\()_isr
.global \name\()_isr
.extern \name\()_handler
.type \name\()_isr, @function
.endm

#endif

