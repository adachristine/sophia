#include <kc.h>

.section .text.start

.hidden kc_entry
.global kc_entry
.type kc_entry, @function
kc_entry:
    push %rbp
    push %rdi
    push %rsi
    mov %rsp, %rbp
    and $-0x10, %rsp
    lea kc_image_base(%rip), %rdi
    lea _DYNAMIC(%rip), %rsi
    call kc_dynamic_init
    mov %rbp, %rsp
    pop %rsi
    pop %rdi
    mov %rsp, %rbp
    and $-0x10, %rsp
    call kc_main
    mov %rbp, %rsp
    pop %rbp
    ret
.size kc_entry, . - kc_entry

.section .text

.hidden kc_resolve_symbol
.global kc_resolve_symbol
.type kc_resolve_symbol, @function
kc_resolve_symbol:
    cli
    hlt
    jmp kc_resolve_symbol
.size kc_resolve_symbol, . - kc_resolve_symbol

