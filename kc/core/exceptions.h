#pragma once

extern void page_fault_isr(void);
extern void general_protection_isr(void);

#define GENERAL_PROTECTION_VECTOR 0xd
#define PAGE_FAULT_VECTOR 0xe

void exceptions_init(void);

