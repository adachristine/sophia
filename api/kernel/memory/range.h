#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uintptr_t phys_addr_t;

enum memory_range_type
{
    UNUSABLE_MEMORY,
    RESERVED_MEMORY,
    SYSTEM_MEMORY,
    AVAILABLE_MEMORY,
};

struct memory_range
{
    enum memory_range_type type;
    phys_addr_t base;
    size_t size;
};
