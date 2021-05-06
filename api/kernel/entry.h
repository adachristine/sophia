#pragma once

#define KERNEL_ENTRY_STACK_SIZE 0x4000
#define KERNEL_ENTRY_STACK_HEAD (void *)0xffffffffff000000
#define KERNEL_ENTRY_STACK_BASE ((char *)KERNEL_ENTRY_STACK_HEAD - KERNEL_ENTRY_STACK_SIZE)

typedef void (*kernel_entry_func)(void *data);
