#pragma once

#define KERNEL_ENTRY_STACK_SIZE 0x8000
#define KERNEL_ENTRY_STACK_HEAD 0xffffffffff000000
#define KERNEL_ENTRY_STACK_BASE (KERNEL_ENTRY_STACK_HEAD - KERNEL_ENTRY_STACK_SIZE)

typedef void (*kernel_entry_func)(void *data);
