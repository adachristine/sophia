#include <kc.h>

.section .text.start

.hidden kc_entry
.global kc_entry
.type kc_entry, @function
kc_entry:
    push %rdi
    push %rsi
    lea kc_image_base(%rip), %rdi
    lea _DYNAMIC(%rip), %rsi
    call kc_dynamic_init
    pop %rsi
    pop %rdi
    call kc_main
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

