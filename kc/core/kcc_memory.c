#include "memory.h"

#include <core/memory.h>

KC_EXPORT
kc_phys_addr kcc_page_alloc(void)
{
    return page_alloc(PAGE_ALLOC_ANY);
}

KC_EXPORT
void kcc_page_free(kc_phys_addr page)
{
    page_free(page);
}

