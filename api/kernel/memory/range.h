#pragma once

#include <stdint.h>
#include <stddef.h>

typedef uintptr_t phys_addr_t;

enum memory_range_type
{
    // types for physical pages
    RESERVED_MEMORY,
    SYSTEM_MEMORY,
    AVAILABLE_MEMORY,
    // types for firmware pages with unknown backing
    FIRMWARE_MEMORY,
    // types for other addresses that are not physical
    MMIO_MEMORY,
    INVALID_MEMORY = 0xff
};

struct memory_range
{
    enum memory_range_type type;
    phys_addr_t base;
    size_t size;
};

