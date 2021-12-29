#pragma once

#include <kernel/memory/paging.h>
#include <kernel/memory/range.h>

#include <stddef.h>

typedef int (*memory_space_handler_func)(uint32_t code, void *vaddr);

enum page_map_flags
{
    CONTENT_CODE = 0x1,
    CONTENT_RODATA = 0x2,
    CONTENT_RWDATA = 0x3,
    CONTENT_MASK = 0x3,
    
    SIZE_4K = 0x4,
    SIZE_2M = 0x8,
    SIZE_1G = 0xc,
    SIZE_MASK = 0xc,
};

void memory_init(struct memory_range *ranges, size_t count);

void *memcpy(void *dest, const void *src, size_t size);
void *memmove(void *dest, const void *src, size_t size);
void *memset(void *dest, int val, size_t size);
int memcmp(const void *str1, const void *str2, size_t count);

void *page_map(phys_addr_t paddr, enum page_map_flags flags);
void page_unmap(void *vaddr);

phys_addr_t page_alloc(void);
void page_free(phys_addr_t paddr);

void *heap_alloc(size_t size);
void heap_free(void *block);

void *memory_alloc(size_t size);
void memory_free(void *block);

