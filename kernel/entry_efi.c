#include <boot/entry_efi.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include "cpu.h"
#include "serial.h"
#include "kprint.h"
#include "memory.h"

static noreturn void hang(void);

static void free_memory_descriptor(EFI_MEMORY_DESCRIPTOR *desc)
{
    kputs("free_memory_descriptor() called\r\n");
    size_t size = desc->NumberOfPages * EFI_PAGE_SIZE;

    do
    {
        size -= EFI_PAGE_SIZE;
        page_free(desc->PhysicalStart + size);
    }
    while (size);
}

static void parse_efi_memmap(struct efi_memory_map_data *map)
{
    int entries = map->size / map->descsize;

    for (int i = 0; i < entries; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc;
        desc = (EFI_MEMORY_DESCRIPTOR *)((char *)map->data + i * map->descsize);

        switch (desc->Type)
        {
            case EfiConventionalMemory:
            case EfiBootServicesData:
            case EfiBootServicesCode:
            case EfiLoaderData:
            case EfiLoaderCode:
                free_memory_descriptor(desc);
                break;
            default:
                break;
        }
    }
}

noreturn void kernel_entry(struct efi_boot_data *data)
{
    cpu_init();
    serial_init();
    kputs("<hacker voice> i'm in\r\n");
    parse_efi_memmap(data->memory_map);
    hang();
}

static noreturn void hang(void)
{
    __asm__ ("cli\n\t");
    while (true) __asm__ ("hlt\n\t");
}

