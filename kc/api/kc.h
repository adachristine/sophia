#pragma once

extern unsigned char kc_image_base;
extern unsigned char kc_text_begin;
extern unsigned char kc_text_end;
extern unsigned char kc_rodata_begin;
extern unsigned char kc_data_begin;
extern unsigned char kc_data_end;
extern unsigned char kc_image_end;

#define KC_EXPORT __attribute__((visibility("default")))

