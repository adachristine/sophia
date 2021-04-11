#pragma once

extern void page_fault_isr(void);

#define PAGE_FAULT_VECTOR 0xe

void exceptions_init(void);
