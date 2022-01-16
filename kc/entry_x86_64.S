.section .text.start
.extern kc_reloc
.extern kc_main
.extern _DYNAMIC
.extern kc_image_base

.hidden kc_entry
.global kc_entry
.type kc_entry, @function
kc_entry:
    push %rdi
    lea _DYNAMIC(%rip), %rdi
    lea kc_image_base(%rip), %rsi
    call kc_reloc
    pop %rdi
    call kc_main
    ret
.size kc_entry, . - kc_entry
    
