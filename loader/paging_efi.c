#include "loader_efi.h"

#include <kernel/memory/paging.h>

#define KERNEL_SPACE_LOWER (void *)0xffffffff80000000
#define KERNEL_SPACE_UPPER (void *)0xffffffffc0000000

static uint64_t *new_page_table(void);
static uint64_t *get_page_table(void *vaddr, int level);
static uint64_t *get_page_entry(void *vaddr);

static uint64_t *get_page_map(void);
static void set_page_map(uint64_t *map);

static uint64_t *efi_page_map = NULL;
static uint64_t *system_page_map = NULL;

void paging_init(void)
{
    efi_page_map = get_page_map();
    system_page_map = new_page_table();
    
    if (!system_page_map)
    {
        Print(L"error creating page map: %r\r\n", e_last_error);
        efi_exit(e_last_error);
    }
    
    // copy the identity mappings from the EFI PML4.
    system_page_map[0] = efi_page_map[0];
    
    // create kernel fractal mappings
    // TODO: move this into the kernel instead?
    uint64_t *lower_pm2 = get_page_table(KERNEL_SPACE_LOWER, 2);
    uint64_t *upper_pm2 = get_page_table(KERNEL_SPACE_UPPER, 2);
    
    upper_pm2[PAGE_TABLE_INDEX_MASK - 1] = (phys_addr_t)lower_pm2|DATA_PAGE_TYPE;
    upper_pm2[PAGE_TABLE_INDEX_MASK] = (phys_addr_t)upper_pm2|DATA_PAGE_TYPE;
}

void paging_enter(void)
{
    // use the system_page_map instead of efi_page_map
    set_page_map(system_page_map);
}

void *paging_map_page(void *vaddr, phys_addr_t paddr, enum page_type type)
{
    uint64_t *entry = get_page_entry(vaddr);
    
    if (entry)
    {
        *entry = paddr | type;
        return vaddr;
    }
    
    return NULL;
}

void *paging_map_pages(void *vaddr,
                       phys_addr_t paddr,
                       enum page_type type,
                       size_t size)
{
    for (size_t offset = 0; offset < size; offset += page_size(1))
    {
        if (!paging_map_page((char *)vaddr + offset, paddr + offset, type))
        {
            return NULL;
        }
    }
    
    return vaddr;
}

void *paging_map_range(void *vaddr,
                       struct memory_range *range,
                       enum page_type type)
{
    return paging_map_pages(vaddr, range->base, type, range->size);
}

static uint64_t *new_page_table(void)
{
    struct memory_range table = system_allocate(page_size(1));
    
    if (table.type == SYSTEM_MEMORY)
    {
        e_bs->SetMem((void *)table.base, table.size, 0);
        return (uint64_t *)table.base;
    }

    return NULL;
}

static uint64_t *get_page_table(void *vaddr, int level)
{
    // don't try to do anything if there's no page map.
    if (!system_page_map)
    {
        return NULL;
    }
    
    // don't try to do anything outside of the page map
    if ((level < 1) || (level > PAGE_MAP_LEVELS))
    {
        return NULL;
    }
    
    // don't use a non-canonical address
    
    uint64_t *map = system_page_map;

    for (int i = PAGE_MAP_LEVELS; i > level; i--)
    {
        if (!map[pte_index(vaddr, i)])
        {
            uint64_t *next_map = new_page_table();
            map[pte_index(vaddr, i)] = (phys_addr_t)next_map|PAGE_PR|PAGE_WR;
            map = next_map;
        }
        else
        {
            map = (uint64_t *)page_address(map[pte_index(vaddr, i)], 1);
        }
    }

    return map;
}

static uint64_t *get_page_entry(void *vaddr)
{
    uint64_t *table = get_page_table(vaddr, 1);
    return &table[pte_index(vaddr, 1)];
}

static uint64_t *get_page_map(void)
{
    uint64_t map;
    __asm__("mov %%cr3, %0" : "=r"(map));
    return (uint64_t *)(page_address(map, 1));
}

static void set_page_map(uint64_t *map)
{
    __asm__
    (
        "pushf\r\n"
        "cli\r\n"
        "mov %0, %%cr3\r\n"
        "popf"
        :
        : "r"(map)
    );
}
