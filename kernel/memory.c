#include "memory.h"

#include <kernel/memory/paging.h>

/*
 * kernel virtual space guarantees
 *
 * 1. there are two kernel page directories, one each for the lower and upper
 *    GiB of space. they are self-mapped as tables in the trailing entries
 *    of the table for the upper space. these must exist.
 * 2. because of (1.) the kernel page tables (directoires inclusive) occupy
 *    the upper 4MiB of kernel address space.
 * 3. the kernel is always at the beginning of its virtual space.
 *    currently this is 0xffffffff80000000 but that may change
 * 4. the entry stack's head is always -4MiB from the end of the address space.
 *    its size is currently 64KiB. this may change.
 */

#define kpde_index(v) ((v >> (PAGE_MAP_BITS + PAGE_OFFSET_BITS)) & 0x3ff)
#define kpte_index(v) ((v >> PAGE_OFFSET_BITS) & 0x7ffff)

extern char k_virt_base;
extern char k_text_begin;
extern char k_text_end;
extern char k_data_begin;
extern char k_data_end;

static uint64_t *const kernel_ptes = (uint64_t *)0xffffffffffc00000;
static uint64_t *const kernel_pdes = (uint64_t *)0xffffffffffffe000;

// we can track 4GiB of physical pages with this bitmap.
#define PAGE_BITMAP_ENTRIES 16384
#define PAGE_BITMAP_WIDTH 64
static uint64_t page_bitmap[PAGE_BITMAP_ENTRIES];

static phys_addr_t bitmap_alloc(void)
{
    static uint64_t *page_bitmap_last = NULL;

    if (!page_bitmap_last || !*page_bitmap_last)
    {
        for (int i = 0; i < PAGE_BITMAP_ENTRIES; i++)
        {
            if (page_bitmap[i])
            {
                page_bitmap_last = &page_bitmap[i];
                break;
            }
        }
    }

    int index = page_bitmap_last - page_bitmap;
    int bit = __builtin_ffsll(*page_bitmap_last);

    return (index * PAGE_BITMAP_WIDTH + bit) * PAGE_SIZE;
}

static void bitmap_free(phys_addr_t page)
{
    int index = (page >> PAGE_OFFSET_BITS) / PAGE_BITMAP_WIDTH;
    int bit = (page >> PAGE_OFFSET_BITS) % PAGE_BITMAP_WIDTH;
    page_bitmap[index] |= (1 << bit);
}
/*
static virt_addr_t virtual_alloc(void)
{
}

static void virtual_free(virt_addr_t vpage)
{
}
*/
static uint64_t *get_kernel_pde(virt_addr_t vaddr)
{
    return &kernel_pdes[kpde_index(vaddr)];
}

static uint64_t *get_kernel_pte(uint64_t vaddr)
{
    return &kernel_ptes[kpte_index(vaddr)];
}

void memory_init()
{
}

phys_addr_t page_alloc(void)
{
    return bitmap_alloc();
}

void page_free(phys_addr_t page)
{
    bitmap_free(page);
}

