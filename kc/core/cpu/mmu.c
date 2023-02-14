#include "mmu.h"
#include "irq.h"

#include <kernel/memory/paging.h>

void mmu_set_map(phys_addr_t map)
{
    uint64_t flags = irq_lock();
    __asm__ volatile ("mov %0, %%cr3" :: "r"(page_address(map, 1)));
    irq_unlock(flags);
}

phys_addr_t mmu_get_map(void)
{
    phys_addr_t pm4_phys;
    __asm__ volatile
        (
         "mov %%cr3, %0\n\t"
         : "=r"(pm4_phys)
        );
    return page_address(pm4_phys, 1);
}

void mmu_invalidate(void *addr)
{
    __asm__ volatile ("invlpg (%0)" :: "r"(addr));
}

