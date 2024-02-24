#pragma once

#ifndef __ASSEMBLER__

#include <elf/elf64.h>

#include <stddef.h>

#include <kjarna/interface.h>

extern struct kjarna_entry_params *entry_params;

#define KC_EXPORT __attribute__((visibility("default")))

extern unsigned char kc_image_base;
extern unsigned char kc_text_begin;
extern unsigned char kc_text_end;
extern unsigned char kc_rodata_begin;
extern unsigned char kc_data_begin;
extern unsigned char kc_data_end;
extern unsigned char kc_image_end;

struct elf64_dso_info;
struct elf64_relatab;

extern void kc_dynamic_init(char *base, Elf64_Dyn *dyn);
extern void *kc_dynamic_link(struct elf64_dso_info *dso, int index);
extern void kc_dynamic_reloc(char *base, struct elf64_relatab *dso);

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
.extern kc_dynamic_link

#endif // __ASSEMBLER__

