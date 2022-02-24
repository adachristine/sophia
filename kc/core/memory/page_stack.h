#pragma once

#include "memory.h"

void page_stack_init(void);

kc_phys_addr page_stack_alloc(enum page_alloc_flags type);
void page_stack_free(kc_phys_addr page);

int page_stack_get_present(kc_phys_addr page);
void page_stack_set_present(kc_phys_addr page);

void page_stack_set_allocated(kc_phys_addr page);
void page_stack_set_free(kc_phys_addr page);

int page_stack_inc_ref(kc_phys_addr page);
int page_stack_dec_ref(kc_phys_addr page);

