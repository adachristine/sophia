#include "cpu.h"
#include "serial.h"
#include "kprint.h"
#include "memory.h"
#include "panic.h"

#include <loader/data_efi.h>

#include <kernel/entry.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

static void efi_memory_init(struct efi_memory_map_data *map)
{
    // FIXME: bad girl shit: overwriting on aliased pointers.
    // this is a temporary solution to shortcomings with
    // loader boot data. it's sort-of OK because the memory_range struct
    // is smaller than EFI_MEMORY_DESCRIPTOR. it's still not good.
    struct memory_range *ranges = (struct memory_range *)map->data;
    int entries = map->size / map->descsize;

    for (int i = 0; i < entries; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc;
        desc = (EFI_MEMORY_DESCRIPTOR *)((char *)map->data + i * map->descsize);

        enum memory_range_type type;
        phys_addr_t base;
        size_t size;
        
        base = desc->PhysicalStart;
        size = desc->NumberOfPages * EFI_PAGE_SIZE;
        
        switch (desc->Type)
        {
            case EfiConventionalMemory:
            case EfiBootServicesData:
            case EfiBootServicesCode:
            case EfiLoaderData:
            case EfiLoaderCode:
                type = AVAILABLE_MEMORY;
                break;
            case SystemMemoryType:
                type = SYSTEM_MEMORY;
                break;
            default:
                type = RESERVED_MEMORY;
        }
        
        ranges[i].type = type;
        ranges[i].base = base;
        ranges[i].size = size;
    }
    
    memory_init(ranges, entries);
}

noreturn void kernel_entry(struct efi_boot_data *data)
{
    (void)data;
    cpu_init();
    serial_init();
    kputs("<hacker voice> i'm in\r\n");
    efi_memory_init(data->memory_map);
    halt();
}
