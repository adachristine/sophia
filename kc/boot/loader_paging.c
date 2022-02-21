#include "loader_paging.h"

uint64_t *get_page_map(void)
{
    uint64_t map;
    __asm__("mov %%cr3, %0" : "=r"(map));
    return (uint64_t *)(page_address(map, 1));
}

void set_page_map(uint64_t *map)
{
    __asm__ volatile
        (
         "pushf\n\t"
         "cli\n\t"
         "mov %0, %%cr3\n\t"
         "popf\n\t"
         :
         : "r"(map));
}

uint64_t *get_page_table(uint64_t *map, uint64_t virt, int level)
{
    if ((level < 1) || (level > PAGE_MAP_LEVELS))
    {
        return NULL;
    }

    for (int i = PAGE_MAP_LEVELS; i > level; i--)
    {
        if (!map[pte_index(virt, i)])
        {
            map[pte_index(virt, i)] =
                (uint64_t)new_page_table()|PAGE_PR|PAGE_WR;
        }

        map = (uint64_t *)page_address(map[pte_index(virt, i)], 1);
    }

    return map;
}

uint64_t *get_page_entry(uint64_t *map, uint64_t virt)
{
    int level = 1;
    uint64_t *table = get_page_table(map, virt, level);
    return &table[pte_index(virt, level)];
}

void map_page(void *map,
        uint64_t virt,
        uint64_t phys,
        enum page_type type)
{
    *get_page_entry(map, virt) = phys | type;
}

void map_pages(void *map,
        uint64_t virt,
        uint64_t phys,
        enum page_type type,
        size_t size)
{
    for (size_t offset = 0; offset < size; offset += page_size(1))
    {
        map_page(map, virt + offset, phys + offset, type);
    }
}

