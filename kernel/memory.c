#include "memory.h"
#include "kprint.h"

#include <kernel/memory/paging.h>

/* kernel virtual space guarantees
 * 
 * the loader must set up the address space as follows
 * 1. kernel virtual space is 2GiB in size on 2GiB alignment.
 * 2. mappings begin at +0x7fc00000. this mapping space is sparse and its
 *    own mapping tables begin at +0x7fffe000.
 */

#define kpm1_index(v) (((uint64_t)v >> page_table_index_bits(1)) & 0x7ffff)
#define kpm2_index(v) (((uint64_t)v >> page_table_index_bits(2)) & 0x3ff)

#define align_next(v, a) (((uint64_t)v + a - 1) & ~(a - 1))

struct page
{
    uint32_t next: 31;
    uint32_t used: 1;
};

extern char k_virt_base;
extern char k_text_begin;
extern char k_text_end;
extern char k_data_begin;
extern char k_data_end;

static struct page *const page_array = (struct page *)0xffffffd800000000;
static int first_free_page_index = -1;
static size_t page_array_entries = 0;

// the temporary mapping place. never use this permanently.
static void *const temp = (void *)0xffffffffffa00000;
static uint64_t *const kernel_pm1 = (uint64_t *)0xffffffffffc00000;
static uint64_t *const kernel_pm2 = (uint64_t *)0xffffffffffffe000;

static uint64_t *get_kernel_pm1e(void *vaddr);
static uint64_t *get_kernel_pm2e(void *vaddr);

static struct memory_range init_grab_pages(struct memory_range *ranges,
                                           int count,
                                           size_t size)
{
    struct memory_range request = {SYSTEM_MEMORY,
                                   0,
                                   align_next(size, PAGE_SIZE)};
    
    for (int i = 0; i < count; i++)
    {
        if (ranges[i].type != AVAILABLE_MEMORY ||
            ranges[i].base < (1 << 20)) // leave pages below 1MiB alone.
        {
            continue;
        }
        else if (ranges[i].size > request.size)
        {
            request.base = ranges[i].base;
            ranges[i].base += request.size;
            ranges[i].size -= request.size;
            break;
        }
    }
    
    return request;
}

static size_t init_get_max_paddr(struct memory_range *ranges, int count)
{
    phys_addr_t max_paddr = 0;

    // get the highest usable physical address in all memory ranges.
    for (int i = 0; i < count; i++)
    {
        if (ranges[i].type != AVAILABLE_MEMORY)
        {
            continue;
        }
        
        if ((ranges[i].base + ranges[i].size - 1) > max_paddr)
        {
            max_paddr = ranges[i].base + ranges[i].size - 1;
        }
    }
    
    return max_paddr;
}

static phys_addr_t get_kernel_pm4_phys(void)
{
    phys_addr_t pm4_phys;
    __asm__ (
             "mov %%cr3, %0\n\t"
             "and %0, %1\n\t"
             : "=r"(pm4_phys)
             : "r"((uint64_t)PAGE_ADDRESS_MASK)
    );
    
    return pm4_phys;
}

static phys_addr_t get_kernel_pm3_phys(void)
{
    phys_addr_t pm3_phys;
    // make a temporary mapping to read pm4
    uint64_t *pm4 = kernel_map_page_at(temp,
                                       get_kernel_pm4_phys(),
                                       CONTENT_RODATA|SIZE_2M);
    
    // pm3_phys is in pm4.
    pm3_phys = pm4[page_table_index(&k_virt_base, 4)] & PAGE_ADDRESS_MASK;
    // never leave a temporary mapping
    kernel_unmap_page(pm4);
    
    return pm3_phys;
}

static void init_map_page_array_pm2(struct memory_range *pm2_pages)
{
    // now we need to write the physical address of each pm2 page for the
    // page_array in the kernel's pm3.
    uint64_t *pm3 = kernel_map_page_at(temp,
                                      get_kernel_pm3_phys(),
                                      CONTENT_RWDATA|SIZE_2M);
    
    // the first index is NOT zero!
    for (size_t i = 0; i < (pm2_pages->size / PAGE_SIZE); i++)
    {
        pm3[page_table_index(page_array, 3) + i] = (pm2_pages->base + i *
                                                    PAGE_SIZE)|PAGE_NX|PAGE_WR|
                                                    PAGE_PR;
    }
    // never forget to unmap temporary mappings.
    kernel_unmap_page(pm3);
}

static void init_create_page_array_map(struct memory_range *pages,
                                       struct memory_range *maps)
{
    // pm1 indices for page_array always start at 0
    size_t pm1_entry_count = pages->size / PAGE_SIZE;
    
    // every 512 entries we need to re-map and clean.
    for (size_t i = 0; i < pm1_entry_count; i += PAGE_TABLE_INDEX_MASK)
    {
        uint64_t *pm1 = kernel_map_page_at(temp,
                                           maps->base + i * PAGE_SIZE,
                                           CONTENT_RWDATA|SIZE_2M);
        memset(pm1, 0, PAGE_SIZE);
        for (size_t j = 0;
             (j < PAGE_TABLE_INDEX_MASK) && ((j + i) < pm1_entry_count);
             j++)
        {
            pm1[j] = (pages->base + (i + j) * PAGE_SIZE)|
                     PAGE_NX|PAGE_WR|PAGE_PR; 
        }
        
        kernel_unmap_page(pm1);
    }
}

static void init_set_memory_range(struct memory_range *range)
{
    for (size_t i = 0; i < range->size / PAGE_SIZE; i++)
    {
        if (range->base / PAGE_SIZE > page_array_entries)
        {
            return;
        }
        
        struct page *page = &page_array[range->base / PAGE_SIZE + i];
        if (range->type == AVAILABLE_MEMORY)
        {
            page->used = 0;
            page->next = first_free_page_index;
            first_free_page_index = range->base / PAGE_SIZE + i;
        }
        else
        {
            page->used = 1;
        }
    }
}

static void init_populate_page_array(struct memory_range *ranges, int count)
{
    for (int i = 0; i < count; i++)
    {
        init_set_memory_range(&ranges[i]);
    }
}

static void init_create_page_array(struct memory_range *ranges, int count)
{
    // the highest actual physical memory address.
    size_t max_paddr = init_get_max_paddr(ranges, count);
    page_array_entries = max_paddr / PAGE_SIZE;
    size_t page_array_size = page_array_entries * sizeof(struct page);
    
    // the physical pages that will contain page_array
    struct memory_range pa_pages = init_grab_pages(ranges, count, page_array_size);
    
    // the number of page tables needed to map pa_pages
    // this will be 1 on systems with less than 2GB of memory.
    size_t pm1_count = align_next(pa_pages.size, PAGE_SIZE_LARGE) / PAGE_SIZE_LARGE;
    struct memory_range pm1_pages = init_grab_pages(ranges,
                                                    count,
                                                    pm1_count * PAGE_SIZE);
    
    init_create_page_array_map(&pa_pages, &pm1_pages);
    
    // the number of page directories needed to map pa_pages
    // this will be 1 on systems with less than 1TB of memory.
    // who the fuck has 1TB of memory lol
    size_t pm2_count = align_next(pa_pages.size, PAGE_SIZE_HUGE) / PAGE_SIZE_HUGE;
    struct memory_range pm2_pages = init_grab_pages(ranges,
                                                    count,
                                                    pm2_count * PAGE_SIZE);
    
    init_create_page_array_map(&pm1_pages, &pm2_pages);
    
    // ok here goes
    init_map_page_array_pm2(&pm2_pages);
    
    memset(page_array, 0, page_array_size);
    
    init_populate_page_array(ranges, count);
    init_populate_page_array(&pa_pages, 1);
    init_populate_page_array(&pm1_pages, 1);
    init_populate_page_array(&pm2_pages, 1);
}

void memory_init(struct memory_range *ranges, int count)
{
    init_create_page_array(ranges, count);
}

void *kernel_map_page_at(void *vaddr,
                         phys_addr_t paddr,
                         enum map_page_flags flags)
{
    uint64_t entry = PAGE_PR;
    
    switch (flags & CONTENT_MASK)
    {
        case CONTENT_RODATA:
            entry |= PAGE_NX;
            break;
        case CONTENT_RWDATA:
            entry |= PAGE_NX|PAGE_WR;
            break;
        default:
            break;
    }
    
    size_t offset;
    uint64_t *pte;
    
    switch (flags & SIZE_MASK)
    {
        case SIZE_2M:
            entry |= (paddr & PAGE_ADDRESS_MASK_LARGE)|PAGE_LG;
            offset = paddr % PAGE_SIZE_LARGE;
            pte = get_kernel_pm2e(vaddr);
            break;
        case SIZE_4K:
            entry |= (paddr & PAGE_ADDRESS_MASK);
            offset = paddr % PAGE_SIZE;
            pte = get_kernel_pm1e(vaddr);
            break;
        default:
            offset = 0;
            pte = NULL;
    }
    
    if (pte)
    {
        *pte = entry;
        return (char *)vaddr + offset;
    }
    
    return NULL;
}

void kernel_unmap_page(void *vaddr)
{
    uint64_t *pte = get_kernel_pm2e(vaddr);
    
    if (pte && !(*pte & PAGE_LG))
    {
        pte = get_kernel_pm1e(vaddr);
    }
    
    if (pte)
    {
        *pte = 0;
    }
    __asm__ ("invlpg (%0)" :: "r"(vaddr));
}

phys_addr_t page_alloc(void)
{
    int index = first_free_page_index;
    
    if (index > 0)
    {
        page_array[index].used = 1;
        first_free_page_index = page_array[index].next;
    }
    
    return (phys_addr_t)index * PAGE_SIZE;
}

void page_free(phys_addr_t page)
{
    int index = page / PAGE_SIZE;
    
    if ((size_t)index > page_array_entries)
    {
        return;
    }
    
    page_array[index].used = 0;
    page_array[index].next = first_free_page_index;
    first_free_page_index = index;
}

static uint64_t *get_kernel_pm1e(void *vaddr)
{
    return &kernel_pm1[kpm1_index(vaddr)];
}

static uint64_t *get_kernel_pm2e(void *vaddr)
{
    return &kernel_pm2[kpm2_index(vaddr)];
}

int page_fault_handler(uint32_t code, void *address)
{
    (void)code;
    (void)address;
    kputs("page faulmt\n");
    return 0;
}
