#pragma once

#include "memory.h"

void mmu_set_map(phys_addr_t map);
phys_addr_t mmu_get_map(void);

void mmu_invalidate(void *addr);

