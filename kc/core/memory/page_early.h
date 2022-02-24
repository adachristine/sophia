#pragma once

#include "memory.h"

void page_early_init(void);
void page_early_final(void);
kc_phys_addr page_early_alloc(enum page_alloc_flags type);

