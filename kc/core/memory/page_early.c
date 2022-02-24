#include "page_early.h"
#include "kprint.h"
#include "panic.h"

static struct page_early_state
{
    struct memory_range *first;
    struct memory_range *current;
    struct memory_range *last;
}
early_state;

void page_early_init(void)
{
    kprintf("initializing early page frame allocator\n");
    struct kc_boot_data *boot_data = get_boot_data();

    early_state.first = &boot_data->memory.entries[0];
    early_state.last = &boot_data->memory.entries[boot_data->memory.count];
    early_state.current = early_state.first;
}

void page_early_final(void)
{
    if (early_state.first)
    {
        kprintf("finalizing early page allocator\n");
        for (struct memory_range *current = early_state.first; 
                current < early_state.last;
                current++)
        {
            while (current->size >= page_size(1))
            {
                current->size -= page_size(1);

                enum memory_range_type type = current->type;
                kc_phys_addr page = current->base + current->size;
                // all pages are gonna be set allocated first
                // to initialize the tracking structure at the other side
                // and make setting the page free as simple as 
                // calling page_free();
                page_set_allocated(page);

                switch (type)
                {
                    case RESERVED_MEMORY:
                    case SYSTEM_MEMORY:
                        page_set_present(page);
                        break;
                    case AVAILABLE_MEMORY:
                        page_set_present(page);
                        page_free(page);
                        break;
                    case FIRMWARE_MEMORY:
                    case MMIO_MEMORY:
                        break;
                    default:
                        break;
                }
            }
        }

        early_state = (struct page_early_state){NULL, NULL, NULL};
    }
}

kc_phys_addr page_early_alloc(enum page_alloc_flags type)
{
    // TODO: support low/conv/high allocations in early_alloc.
    // currently we only have conventional allocations enforced
    // by the conditions of the scanning loop
    (void)type;

    while(early_state.current)
    {
        if ((early_state.current->type == AVAILABLE_MEMORY) &&
                (early_state.current->base > 0x10000) &&
            (early_state.current->size > page_size(1)))
        {
            early_state.current->size -= page_size(1);
            return early_state.current->base + early_state.current->size;
        }

        if (early_state.current == early_state.last)
        {
            early_state.current = NULL;
            break;
        }
        early_state.current++;
    }

    kprintf("error: early allocator has run out of memory\n");
    PANIC(OUT_OF_MEMORY);
}

