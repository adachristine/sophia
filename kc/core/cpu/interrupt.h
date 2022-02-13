#pragma once

#ifndef __ASSEMBLER__
#else

.macro ISR_STORE_PARAM_REGS
push %rdi
push %rsi
push %rdx
push %rcx
push %r8
push %r9
.endm

#define ISR_PARAM_REGS_SIZE 48

.macro ISR_STORE_CALLER_REGS
push %rax
push %r10
push %r11
.endm

#define ISR_CALLER_REGS_SIZE 24

.macro ISR_STORE_CALLEE_REGS
push %rbx
push %rbp
push %r12
push %r13
push %r14
push %r15
.endm

#define ISR_CALLEE_REGS_SIZE 48

.macro ISR_RESTORE_PARAM_REGS
pop %r9
pop %r8
pop %rcx
pop %rdx
pop %rsi
pop %rdi
.endm

.macro ISR_RESTORE_CALLER_REGS
pop %r11
pop %r10
pop %rax
.endm

.macro ISR_RESTORE_CALLEE_REGS
pop %r15
pop %r14
pop %r13
pop %r12
pop %rbp
pop %rbx
.endm

.macro ISR_CALL func
    // align the stack for the call to the handler function
    mov %rsp, %rbp
    and $-16, %rsp
    call \func
    // restore stack pointer, possibly unaligned
    mov %rbp, %rsp
.endm

#endif

