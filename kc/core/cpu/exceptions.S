#include "exceptions.h"
#include "interrupt.h"

.macro EXCEPTION_BEGIN vec:req, code
    // just in case something tried to be clever before
    pushq $\vec
    .ifb \code
    pushq $0
    .endif
    // push param registers
    ISR_STORE_PARAM_REGS
    // caller-saved registers
    ISR_STORE_CALLER_REGS
    // callee-saved registers
    ISR_STORE_CALLEE_REGS
.endm

.macro EXCEPTION_END vec:req, code
    // restore all the registers
    ISR_RESTORE_CALLEE_REGS
    ISR_RESTORE_CALLER_REGS
    ISR_RESTORE_PARAM_REGS
    add $16, %rsp
    iretq
.endm

.macro EXCEPTION_DEFINE name:req, vec:req
\name\()_isr:
    .if \vec > 9 && \vec < 15 || \vec == 17 || \vec == 21 || \vec > 28 && \vec < 31
    EXCEPTION_BEGIN \vec, 1
    .else
    EXCEPTION_BEGIN \vec
    .endif
    movq %rsp, %rdi
    ISR_CALL \name\()_handler
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

