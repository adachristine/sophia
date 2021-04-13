#include "memory.h"
#include "kprint.h"
#include "panic.h"

#include <kernel/entry.h>
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

// a conservative 128MB for kernel heap
#define KERNEL_HEAP_SPACE_SIZE (128 << 20)

typedef int (*page_fault_handler_func)(uint32_t code, void *address);

enum memory_space_flags
{
    AVAILABLE_MEMORY_SPACE, // memory that is not used
    SYSTEM_MEMORY_SPACE, // memory that cannot fault
    ANONYMOUS_MEMORY_SPACE, // memory that can fault
};

struct memory_space
{
    enum memory_space_flags flags;
    page_fault_handler_func handler;
    void *base;
    void *head;
    struct memory_space *next;
    struct memory_space *prev;
};

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
static size_t free_pages = 0;

// the temporary mapping place. never use this permanently.
static void *const temp = (void *)0xffffffffffa00000;
static uint64_t *const kernel_pm1 = (uint64_t *)0xffffffffffc00000;
static uint64_t *const kernel_pm2 = (uint64_t *)0xffffffffffffe000;

static uint64_t *get_kernel_pm1e(void *vaddr);
static uint64_t *get_kernel_pm2e(void *vaddr);
static void *get_virtual_page(enum page_map_flags flags);
static void *page_map_at(void *vaddr,
                                phys_addr_t paddr,
                                enum page_map_flags flags);

// handlers for memory space types
int anonymous_page_handler(uint32_t code, void *address);

// the system memory_space's that are always present
static struct memory_space kernel_image_space;
static struct memory_space kernel_stack_space;
static struct memory_space kernel_pagemap_space;

// NULL if the system address spaces are not set up
static struct memory_space *root_memory_space;

// the kernel's own heap space
static struct memory_space kernel_heap_space;

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
    uint64_t *pm4 = page_map_at(temp,
                                       get_kernel_pm4_phys(),
                                       CONTENT_RODATA|SIZE_2M);
    
    // pm3_phys is in pm4.
    pm3_phys = pm4[page_table_index(&k_virt_base, 4)] & PAGE_ADDRESS_MASK;
    // never leave a temporary mapping
    page_unmap(pm4);
    
    return pm3_phys;
}

static void init_map_page_array_pm2(struct memory_range *pm2_pages)
{
    // now we need to write the physical address of each pm2 page for the
    // page_array in the kernel's pm3.
    uint64_t *pm3 = page_map_at(temp,
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
    page_unmap(pm3);
}

static void init_create_page_array_map(struct memory_range *pages,
                                       struct memory_range *maps)
{
    // indices for page_array always start at 0
    size_t entry_count = pages->size / PAGE_SIZE;
    
    // every 512 entries we need to re-map and clean.
    for (size_t i = 0; i < entry_count; i += PAGE_TABLE_INDEX_MASK)
    {
        uint64_t *pm1 = page_map_at(temp,
                                           maps->base + i * PAGE_SIZE,
                                           CONTENT_RWDATA|SIZE_2M);
        memset(pm1, 0, PAGE_SIZE);
        for (size_t j = 0;
             (j < PAGE_TABLE_INDEX_MASK) && ((j + i) < entry_count);
             j++)
        {
            pm1[j] = (pages->base + (i + j) * PAGE_SIZE)|
                     PAGE_NX|PAGE_WR|PAGE_PR; 
        }
        
        page_unmap(pm1);
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

static struct memory_space init_system_space(void *base, void *head)
{
    struct memory_space space = {SYSTEM_MEMORY_SPACE, NULL, base, head, NULL, NULL};
    return space;
}

static struct memory_space init_anonymous_space(void *base, void *head)
{
    struct memory_space space = {ANONYMOUS_MEMORY_SPACE,
                                 &anonymous_page_handler,
                                 base,
                                 head,
                                 NULL,
                                 NULL};
    
    return space;
}

static void init_heap_space(struct memory_space *heap)
{
    uint64_t *heap_alias = heap->base;
    *heap_alias = 0xfafafafa;
}

void memory_init(struct memory_range *ranges, int count)
{
    init_create_page_array(ranges, count);
    // initialize always-present memory spaces for the vmm
    kernel_image_space = init_system_space((void *)&k_text_begin,
                                           (void *)align_next(&k_data_end, PAGE_SIZE));
    kernel_stack_space = init_system_space((void *)KERNEL_ENTRY_STACK_BASE,
                                           (void *)KERNEL_ENTRY_STACK_HEAD);
    kernel_pagemap_space = init_system_space(kernel_pm1, (void *)-1LL);
    
    // initialize the kernel heap space
    kernel_heap_space = init_anonymous_space(kernel_image_space.head,
                                             (char *)kernel_image_space.head +
                                             KERNEL_HEAP_SPACE_SIZE);
    
    // set up the list links
    kernel_image_space.next = &kernel_heap_space;
    kernel_heap_space.prev = &kernel_image_space;
    kernel_heap_space.next = &kernel_stack_space;
    kernel_stack_space.prev = &kernel_heap_space;
    kernel_stack_space.next = &kernel_pagemap_space;
    kernel_pagemap_space.prev = &kernel_stack_space;
    
    root_memory_space = &kernel_image_space;
    
    init_heap_space(&kernel_heap_space);
}

static void *page_map_at(void *vaddr,
                                phys_addr_t paddr,
                                enum page_map_flags flags)
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

void *page_map(phys_addr_t paddr, enum page_map_flags flags)
{
    return page_map_at(get_virtual_page(flags), paddr, flags);
}

void page_unmap(void *vaddr)
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
        free_pages--;
    }
    
    return (phys_addr_t)index * PAGE_SIZE;
}

void page_free(phys_addr_t paddr)
{
    int index = paddr / PAGE_SIZE;
    
    if ((size_t)index > page_array_entries)
    {
        return;
    }
    
    page_array[index].used = 0;
    page_array[index].next = first_free_page_index;
    first_free_page_index = index;
    free_pages++;
}

static uint64_t *get_kernel_pm1e(void *vaddr)
{
    return &kernel_pm1[kpm1_index(vaddr)];
}

static uint64_t *get_kernel_pm2e(void *vaddr)
{
    return &kernel_pm2[kpm2_index(vaddr)];
}

static void *get_virtual_page(enum page_map_flags flags)
{
    (void)flags;
    return NULL;
}

static struct memory_space *get_memory_space(void *address)
{
    // walk the memory space list
    struct memory_space *space = root_memory_space;
    
    while (space)
    {
        if (space->base >= address && address <= space->head)
        {
            return space;
        }
        space = space->next;
    }

    return NULL;
}

int anonymous_page_handler(uint32_t code, void *address)
{
    (void)code;
    kputs("anonymous fault\n");
    // kernel page mappings are built different
    if (address >= (void *)&k_virt_base)
    {
        uint64_t *pm2e = get_kernel_pm2e(address);
        
        if (! (*pm2e & PAGE_ADDRESS_MASK))
        {
            // create a page table and install it
            phys_addr_t pm1_phys = page_alloc();
            uint64_t *pm1;
            if (pm1_phys)
            {
                pm1 = page_map_at(temp, pm1_phys, CONTENT_RWDATA|SIZE_2M);
            }
            else
            {
                panic(OUT_OF_MEMORY);
            }
            // zero the page
            memset(pm1, 0, PAGE_SIZE);
            // put the table where it goes
            *pm2e = (pm1_phys & PAGE_ADDRESS_MASK)|PAGE_WR|PAGE_PR;
            page_unmap(pm1);
        }
        
        uint64_t *pm1e = get_kernel_pm1e(address);
        
        if (pm1e)
        {
            phys_addr_t page_phys = page_alloc();
            if (page_phys)
            {
                *pm1e = (page_phys & PAGE_ADDRESS_MASK)|PAGE_WR|PAGE_PR;
            }
        }
    }
    return 0;
}

int page_fault_handler(uint32_t code, void *address)
{
    kputs("page faulmt\n");
    struct memory_space *space = get_memory_space(address);
    if (space && space->handler)
    {
        int result = space->handler(code, address);
        
        if (result)
        {
            panic(UNHANDLED_FAULT);
        }
    }
    
    return 0;
}
