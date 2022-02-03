#pragma once

#include <stddef.h>
#include <kernel/memory/range.h>

typedef int (*kc_entry_func)(void *parameters);

struct kc_boot_data
{
    struct
    {
        void *base;
        size_t size;
    } object_space;
    struct
    {
        struct memory_range *base;
        size_t entries;
    } phys_memory_map;
    struct
    {
        struct memory_range range;
        size_t pitch; // line stride in bytes
        uint16_t width; // width in pixels
        uint16_t height; // height in pixels
        uint8_t format; // pixel format
    } framebuffer_info;
    struct
    {
        void *rsdp;
        uint8_t version;
    } acpi_info;
};

struct kc_boot_data *get_boot_data(void);

