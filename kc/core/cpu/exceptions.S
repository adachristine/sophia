#include "exceptions.h"

.macro EXCEPTION_BEGIN vec:req, code=0
    // just in case something tried to be clever before
    cld
    // push param registers
    push %rdi
    // store certain parameters
    movb $\vec,%dil
    push %rsi
    .if \code
    movq 16(%rsp), %rsi
    .else
    xor %rsi, %rsi
    .endif
    push %rdx
    push %rcx
    push %r8
    push %r9
    // caller-saved registers
    push %rax
    push %r10
    push %r11
    // callee-saved registers
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
.endm

.macro EXCEPTION_END vec:req, code=0
    // restore all the registers
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbp
    pop %rbx
    pop %r11
    pop %r10
    pop %rax
    pop %r9
    pop %r8
    pop %rcx
    pop %rdx
    pop %rsi
    pop %rdi
    .if \code
    add $8, %rsp
    .endif
    iretq
.endm

.macro EXCEPTION_CALL func
    // align the stack for the call to the handler function
    mov %rsp, %rbp
    and $-16, %rsp
    call \func
    // restore stack pointer, possibly unaligned
    mov %rbp, %rsp
.endm

.macro EXCEPTION_DEFINE name:req, vec:req
\name\()_isr:
    .if \vec > 9 && \vec < 15 || \vec == 17 || \vec == 21 || \vec > 28 && \vec < 31
    EXCEPTION_BEGIN \vec, 1
    .else
    EXCEPTION_BEGIN \vec
    .endif
    EXCEPTION_CALL \name\()_handler
    .if \vec > 9 && \vec < 15 || \vec == 17 || \vec == 21 || \vec > 28 && \vec < 31
    EXCEPTION_END \vec, 1
    .else
    EXCEPTION_END \vec
    .endif
.size \name\()_isr, . - \name\()_isr
.endm

EXCEPTION_DECLARE page_fault PAGE_FAULT_EXCEPTION
EXCEPTION_DECLARE general_protection GENERAL_PROTECTION_EXCEPTION
EXCEPTION_DEFINE page_fault, PAGE_FAULT_EXCEPTION
EXCEPTION_DEFINE general_protection, GENERAL_PROTECTION_EXCEPTION

