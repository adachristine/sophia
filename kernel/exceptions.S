.extern page_fault_handler

.global page_fault_isr
.type page_fault_isr, @function
page_fault_isr:
    /* just in case something tried to be clever before */
    cld
    /* push param registers */
    push %rdi
    push %rsi
    push %rdx
    push %rcx
    push %r8
    push %r9
    /* collect parameters for page fault handler */
    mov 48(%rsp), %rdi /* error code is first parameter */
    mov %cr2, %rsi /* PFLA is next parameter */
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
    /* stack has to be aligned */
    mov %rsp, %rbp
    and $-16, %rsp
    call page_fault_handler
    mov %rbp, %rsp
    /* TODO: test for error code here and panic() */
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
    /* unwind the error code from the stack */
    add $8, %rsp
    /* leave the isr */
    iretq
.size page_fault_isr, . - page_fault_isr
    
