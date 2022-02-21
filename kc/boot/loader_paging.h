#pragma once

#include <kernel/memory/paging.h>

#include <stdint.h>

enum page_type
{
    INVALID_PAGE_TYPE = 0x0,
    CODE_PAGE_TYPE = PAGE_PR,
    RODATA_PAGE_TYPE = PAGE_PR,
    DATA_PAGE_TYPE = PAGE_PR|PAGE_WR
};

uint64_t *new_page_table(void);
uint64_t *get_page_map(void);
void set_page_map(uint64_t *map);
uint64_t *get_page_table(uint64_t *map, uint64_t virt, int level);
uint64_t *get_page_entry(uint64_t *map, uint64_t virt);
void map_page(void *map,
        uint64_t virt,
        uint64_t phys,
        enum page_type type);
void map_pages(void *map,
        uint64_t virt,
        uint64_t phys,
        enum page_type type,
        size_t size);

