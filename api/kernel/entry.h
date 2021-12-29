#pragma once

#include <stddef.h>

typedef int (*kc_entry_func)(void *parameters);

struct kc_boot_data
{
    struct
    {
        struct memory_range *base;
        size_t entries;
    }
    phys_memory_map;
};

