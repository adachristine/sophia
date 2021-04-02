#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint64_t phys_addr_t;
typedef uint64_t virt_addr_t;

void memory_init();

void *kmalloc(size_t size);
void kfree(void *block);

phys_addr_t page_alloc(void);
void page_free(phys_addr_t page);

