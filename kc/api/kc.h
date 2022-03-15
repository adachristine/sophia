#pragma once

#ifndef __ASSEMBLER__

#include <elf/elf64.h>

#include <stddef.h>

#define KC_EXPORT __attribute__((visibility("default")))

extern unsigned char kc_image_base;
extern unsigned char kc_text_begin;
extern unsigned char kc_text_end;
extern unsigned char kc_rodata_begin;
extern unsigned char kc_data_begin;
extern unsigned char kc_data_end;
extern unsigned char kc_image_end;

extern void kc_dynamic_init(void *base, Elf64_Dyn *dyn);
extern void kc_reloc(
        void *base,
        Elf64_Rela *entries,
        size_t size,
        size_t entsize);

extern void kc_resolve_symbol(void);

#else

.extern kc_image_base
.extern kc_text_begin
.extern kc_text_end
.extern kc_rodata_begin
.extern kc_rodata_end
.extern kc_data_begin
.extern kc_data_end
.extern kc_image_end
.extern _DYNAMIC

.extern kc_dynamic_init

#endif // __ASSEMBLER__

