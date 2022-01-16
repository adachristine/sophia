/* thread switching routines for x86_64
* adapted from:
* https://wiki.osdev.org/Brendan%27s_Multi-tasking_Tutorial
* 
* 
*/

.hidden cpu_set_thread
.global cpu_set_thread
.type cpu_set_thread, @function
cpu_set_thread:
    /* x86_64 SysV ABI
    *  must save the following registers to the stack:
    *   rbx, rsp, rbp, r12, r13, r14, r15
    *  the following are the parameters for switching tasks
    *   rdi: pointer to control block of thread being switched to 
    *   rsi: pointer to control block of the current thread
    *   rdx: pointer to this cpu's task state segment esp0 field
    */
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    push %rbp
    /* the current task will be in rsi */
    mov %esp, 0(%rsi) // save the stack pointer for the current task
    mov 0(%rdi),%rsp // new stack is the next's task's stack
    mov 8(%rdi),%rbx // top of the next task's kernel stack
    mov %rbx,0(%rdx) // save the esp0 to the tss
    mov 16(%rdi),%rax // page map for the next task
    mov %cr3, %rcx // in case we need to change the page map
    cmp %rax,%rcx
    je .Lfinish
    mov %rax,%cr3 // if the page maps are not the same, reload
.Lfinish:
    pop %rbp // get all the registers off the new task's stack
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    ret // leave into the new task
.size cpu_set_thread, . - cpu_set_thread

