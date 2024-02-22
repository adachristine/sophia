#include <kc.h>

.section .text.start

.extern kjarna_entry

.hidden _start
.global _start
.type _start, @function
_start:
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
	call kjarna_entry
    mov %rbp, %rsp
    pop %rbp
    ret
.size _start, . - _start

.section .text

.hidden _rtld_link
.global _rtld_link
.type _rtld_link, @function
_rtld_link:
	push %rbp
	mov %rsp, %rbp
	push %rdx
	push %rsi
	push %rdi
	mov 8(%rbp), %rdi
	mov 16(%rbp), %rsi
	push %rbp
	mov %rsp, %rbp
	and $-0x10, %rsp
	call kc_dynamic_link
	mov %rbp, %rsp
	pop %rbp
	pop %rdi
	pop %rsi
	pop %rdx
	mov %rbp, %rsp
	pop %rbp
	add $0x10, %rsp
	jmp *(%rax)
.size _rtld_link, . - _rtld_link

