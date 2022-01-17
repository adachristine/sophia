.extern page_fault_handler
.extern general_protection_handler

.macro ISR_STORE_REGISTERS
    /* just in case something tried to be clever before */
    cld
    /* push param registers */
    push %rdi
    push %rsi
    push %rdx
    push %rcx
    push %r8
    push %r9
    /* caller-saved registers */
    push %rax
    push %r10
    push %r11
    /* callee-saved registers */
    push %rbx
    push %rbp
    push %r12
    push %r13
    push %r14
    push %r15
.endm

.macro ISR_RESTORE_REGISTERS
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
.endm

.macro ISR_CALL func
    // align the stack for the call to the handler function
    mov %rsp, %rbp
    and $-16, %rsp
    call \func
    // restore stack pointer, possibly unaligned
    mov %rbp, %rsp
.endm

.hidden page_fault_isr
.global page_fault_isr
.type page_fault_isr, @function
page_fault_isr:
    ISR_STORE_REGISTERS
    /* collect parameters for page fault handler */
    mov 120(%rsp), %rdi /* error code is first parameter */
    mov %cr2, %rsi /* PFLA is next parameter */
    /* stack has to be aligned */
    ISR_CALL page_fault_handler
    /* TODO: test for error code here and panic() */
    ISR_RESTORE_REGISTERS
    /* unwind the error code from the stack */
    add $8, %rsp
    /* leave the isr */
    iretq
.size page_fault_isr, . - page_fault_isr

.hidden general_protection_isr
.global general_protection_isr
.type general_protection_isr, @function
general_protection_isr:
    ISR_STORE_REGISTERS
    // TODO: parameters?
    ISR_CALL general_protection_handler
    add $8, %rsp
    ISR_RESTORE_REGISTERS
    iretq
.size general_protection_isr, . - general_protection_isr

