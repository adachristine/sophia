#include "memory.h"

#include <kc/core/memory.h>

KC_EXPORT
kc_phys_addr kcc_page_alloc(void)
{
    return page_alloc();
}

KC_EXPORT
void kcc_page_free(kc_phys_addr page)
{
    page_free(page);
}

