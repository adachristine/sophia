/*
 * kernel component core - memory management interface
 */
#pragma once

#include <kc.h>

#include <stdint.h>

typedef uint64_t kc_phys_addr;
typedef void * kc_virt_addr;

kc_phys_addr kcc_page_alloc(void);
void kcc_page_free(kc_phys_addr page);

