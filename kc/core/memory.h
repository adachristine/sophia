#pragma once

#include <kernel/entry.h>
#include <kernel/memory/paging.h>
#include <kernel/memory/range.h>

#include <sophialib.h>

#include <core/memory.h>

#include "vm_tree.h"

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

enum page_alloc_flags
{
    PAGE_ALLOC_NONE,
    PAGE_ALLOC_LOW,
    PAGE_ALLOC_CONV,
    PAGE_ALLOC_HIGH,
    PAGE_ALLOC_ANY
};

enum vm_alloc_flags
{
    VM_ALLOC_ANY = 0,
    VM_ALLOC_CACHE = 1,
    VM_ALLOC_CORE = 2,
    VM_ALLOC_TYPE_MASK = 3,

    VM_ALLOC_IMMEDIATE = 4,
    VM_ALLOC_ACTION_MASK = 4,

    VM_ALLOC_ANONYMOUS = 8,
    VM_ALLOC_DIRECT = 16,
    VM_ALLOC_TRANSLATE = 24,
    VM_ALLOC_MECHANISM_MASK = 24,
};

void memory_init(void);

void page_set_present(kc_phys_addr page);
void page_set_allocated(kc_phys_addr page);
void page_set_free(kc_phys_addr page);

int page_inc_ref(kc_phys_addr page);
int page_dec_ref(kc_phys_addr page); 

void *page_map(phys_addr_t paddr, enum page_map_flags flags);
void page_unmap(void *vaddr);

phys_addr_t page_alloc(enum page_alloc_flags flags);
void page_free(phys_addr_t paddr);

void *heap_alloc(size_t size);
void heap_free(void *block);

void *vm_alloc(size_t size, enum vm_alloc_flags flags);
void vm_free(void *block);

void *memory_alloc(size_t size);
void memory_free(void *block);

struct vm_tree *vm_get_tree(void);

int anonymous_page_handler(struct vm_tree_node *, uint32_t, void *);

