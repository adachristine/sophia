#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdnoreturn.h>

#include <boot/entry/entry_efi.h>

#include "serial.h"
#include "kprint.h"

#define ENTRY_STACK_SIZE 0x2000
#define align2(x, a) ((x + a - 1) & ~(a - 1))

extern void kernel_main(void);

enum page_range_type
{
    UNUSABLE_MEMORY,
    RESERVED_MEMORY,
    AVAILABLE_MEMORY
};

struct page_range
{
    uint64_t type;
    uint64_t base;
    size_t size;
};

static char entry_heap[0x10000];
static char *entry_heap_base;
static char *entry_heap_head;

static struct page_range *system_ranges;
static size_t system_ranges_count;

static void entry_heap_init(void)
{
    kputs("entry_heap_init() called\r\n");

    entry_heap_base = entry_heap;
    entry_heap_head = entry_heap;
}

static void *entry_malloc(size_t size)
{
    kputs("entry_malloc() called\r\n");

    void *buffer;

    size = align2(size, 16);

    if ((entry_heap_head - entry_heap_base + size) > sizeof(entry_heap))
    {
        return NULL;
    }
    else
    {
        buffer = entry_heap_head;
        entry_heap_head += size;
    }

    return buffer;
}

static void parse_memory_ranges(struct efi_memory_map_data *map)
{
    kputs("parse_memory_ranges() called\r\n");
    system_ranges_count = map->size/map->descsize;
    system_ranges = entry_malloc(sizeof(*system_ranges) * system_ranges_count);

    for (unsigned int i = 0; i < system_ranges_count; i++)
    {
        EFI_MEMORY_DESCRIPTOR *desc;
        desc = (EFI_MEMORY_DESCRIPTOR *)(map->descsize * i + (char *)map->data);
        system_ranges[i].base = desc->PhysicalStart;
        system_ranges[i].size = desc->NumberOfPages * EFI_PAGE_SIZE;
        
        switch (desc->Type)
        {
            case EfiConventionalMemory:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiLoaderCode:
            case EfiLoaderData:
                system_ranges[i].type = AVAILABLE_MEMORY;
                break;
            case SystemMemoryType:
                system_ranges[i].type = RESERVED_MEMORY;
                break;
            default:
                system_ranges[i].type = UNUSABLE_MEMORY;
        }
    }
}

noreturn void kernel_entry(struct efi_boot_data *data)
{
    serial_init();
    kputs("<hacker voice> i'm in\r\n");
    entry_heap_init();

    parse_memory_ranges(data->memory_map);

    void *acpi_rsdp = data->acpi_rsdp;
    (void)acpi_rsdp;

    // we no longer need any loader data from here on.
    // get the entry stack and clean up loader mappings
    
    char *entry_stack_base = entry_malloc(ENTRY_STACK_SIZE);
    char *entry_stack_head = entry_stack_base + ENTRY_STACK_SIZE;

    __asm__("mov %0, %%rsp" :: "r"(entry_stack_head));

    uint64_t pml4_raw;
    __asm__("mov %%cr3, %0" : "=r"(pml4_raw));
    uint64_t *pml4 = (uint64_t *)(pml4_raw & ~((1ULL << 12) - 1));
    pml4[0] = 0;
    __asm__("mov %0, %%cr3" :: "r"(pml4_raw));

    // we can now go to the kernel proper
    kernel_main();    

    __asm__ ("cli\n\t");
    while (true) __asm__ ("hlt\n\t");
}

