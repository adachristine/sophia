#include "memory.h"
#include "kprint.h"
#include "panic.h"
#include "vm_tree.h"

#include <stdint.h>

#include <kernel/component.h>

/* kernel virtual space guarantees
 * 
 * the loader must set up the address space as follows
 * 1. kernel virtual space is 2GiB in size on 2GiB alignment.
 * 2. mappings begin at +0x7fc00000. this mapping space is sparse and its
 *    own mapping tables begin at +0x7fffe000.
 * 3. There is at least 64KiB of mapped pages following the kc_image_end symbol.
 */

#define kpm1_index(v) (((uint64_t)v >> pte_index_bits(1)) & 0x7ffff)
#define kpm2_index(v) (((uint64_t)v >> pte_index_bits(2)) & 0x3ff)

#define align_next(v, a) (((uint64_t)v + a - 1) & ~(a - 1))

typedef int (*page_fault_handler_func)(uint32_t code, void *address);

enum memory_space_flags
{
    VOID_MEMORY_SPACE = 0x00, // memory that cannot do anything, i.e. a guard page
    TRANSLATION_MEMORY_SPACE = 0x01, // a mapping to a fixed location in memory
    ANONYMOUS_MEMORY_SPACE = 0x02, // a mapping to an arbitrary location in memory
    OBJECT_MEMORY_SPACE = 0x03, // a mapping managed by an underlying object

    MEMORY_SPACE_TYPE_MASK = 0x0f, //

    NOFAULT_MEMORY_FLAG = 0x10, // a mapping that cannot be resolved during a page fault
    NOSWAP_MEMORY_FLAG = 0x20, // a mapping that cannot be swapped out
    COW_MEMORY_FLAG = 0x40, // a mapping that is copy-on-write

    MEMORY_SPACE_FLAGS_MASK = 0xf0,

    INVALID_MEMORY_SPACE = 0xff
};

struct page
{
    uint32_t next: 31;
    uint32_t used: 1;
};

extern char kc_image_base;

static struct page *const page_array = (struct page *)0xffffffd800000000;
static int first_free_page_index = -1;
static size_t page_array_entries = 0;
static size_t free_pages = 0;

// the temporary mapping place. never use this permanently.
static void *const vm_temp = (void *)0xffffffffffa00000;
static uint64_t *const kernel_pm1 = (uint64_t *)0xffffffffffc00000;
static uint64_t *const kernel_pm2 = (uint64_t *)0xffffffffffffe000;

static uint64_t *get_kernel_pm1e(void *vaddr);
static uint64_t *get_kernel_pm2e(void *vaddr);
static void *page_map_at(void *vaddr, phys_addr_t paddr, enum page_map_flags flags);

struct vm_tree core_vm_tree;
struct vm_tree_node core_pagestack_node;
struct vm_tree_node core_image_node;
struct vm_tree_node core_object_node;
struct vm_tree_node core_pagemaps_node;

static struct memory_range init_grab_pages(struct memory_range *ranges,
        int count,
        size_t size)
{
    struct memory_range request = {SYSTEM_MEMORY,
        0,
        align_next(size, page_size(1))};

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
            return request;
        }
    }

    request.type = INVALID_MEMORY;
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
            : "=r"(pm4_phys)
            );

    return page_address(pm4_phys, 1);
}

static phys_addr_t get_kernel_pm3_phys(void)
{
    phys_addr_t pm3_phys;
    // make a temporary mapping to read pm4
    uint64_t *pm4 = page_map_at(vm_temp,
            get_kernel_pm4_phys(),
            CONTENT_RODATA|SIZE_2M);

    // pm3_phys is in pm4.
    pm3_phys = page_address(pm4[pte_index(&kc_image_base, 4)], 1);
    // never leave a temporary mapping
    page_unmap(pm4);

    return pm3_phys;
}

static void init_map_page_array_pm2(struct memory_range *pm2_pages)
{
    // now we need to write the physical address of each pm2 page for the
    // page_array in the kernel's pm3.
    uint64_t *pm3 = page_map_at(vm_temp,
            get_kernel_pm3_phys(),
            CONTENT_RWDATA|SIZE_2M);

    // the first index is NOT zero!
    for (size_t i = 0; i < (pm2_pages->size / PAGE_SIZE); i++)
    {
        pm3[pte_index(page_array, 3) + i] = (
                pm2_pages->base + i *
                PAGE_SIZE)|PAGE_NX|PAGE_WR|PAGE_PR;
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
        uint64_t *pm1 = page_map_at(
                vm_temp,
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
        if (range->base / PAGE_SIZE + i > page_array_entries)
        {
            return;
        }

        if (range->type == AVAILABLE_MEMORY)
        {
            page_free(range->base + PAGE_SIZE * i);
        }
        else
        {
            struct page *page = &page_array[range->base / PAGE_SIZE + i];
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
    page_array_entries = max_paddr / page_size(1);
    size_t page_array_size = page_array_entries * sizeof(struct page);

    // the physical pages that will contain page_array
    struct memory_range pa_pages = init_grab_pages(ranges, count, page_array_size);

    // the number of page tables needed to map pa_pages
    // this will be 1 on systems with less than 2GB of memory.
    size_t pm1_count = page_count(pa_pages.size, 2);
    struct memory_range pm1_pages = init_grab_pages(ranges,
            count,
            pm1_count * page_size(1));

    init_create_page_array_map(&pa_pages, &pm1_pages);

    // the number of page directories needed to map pa_pages
    // this will be 1 on systems with less than 1TB of memory.
    // who the fuck has 1TB of memory lol
    size_t pm2_count = page_count(pa_pages.size, 3);
    struct memory_range pm2_pages = init_grab_pages(ranges,
            count,
            pm2_count * page_size(1));

    init_create_page_array_map(&pm1_pages, &pm2_pages);

    // ok here goes
    init_map_page_array_pm2(&pm2_pages);

    memset(page_array, 0, page_array_size);

    init_populate_page_array(ranges, count);
    init_populate_page_array(&pa_pages, 1);
    init_populate_page_array(&pm1_pages, 1);
    init_populate_page_array(&pm2_pages, 1);
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
            entry |= page_address(paddr, 2)|PAGE_LG;
            offset = page_offset(paddr, 2);
            pte = get_kernel_pm2e(vaddr);
            break;
        case SIZE_4K:
            entry |= page_address(paddr, 1);
            offset = page_offset(paddr, 1);
            pte = get_kernel_pm1e(vaddr);
            break;
        default:
            offset = 0;
            pte = NULL;
    }

    if (!pte)
    {
        return NULL;
    }

    *pte = entry;
    return (char *)vaddr + offset;
}

#define KERNEL_OBJECT_SPACE_EXTENT 0x1000000 // 16MiB for kernel object space?

static void init_vm_node(struct vm_tree_node *node, void *base, void *head)
{
    node->key =
        (struct vm_tree_key)
        {
            (uintptr_t)base,
            (size_t)head - (size_t)base
        };

    if (!vmt_search_key(&core_vm_tree, &node->key))
    {
        kputs("inserting node\n");
        struct vm_tree_node *p = vmn_predecessor_key(
                core_vm_tree.root,
                &node->key);
        vmt_insert(
                &core_vm_tree,
                node,
                p,
                vmn_child_direction(node, p));
    }
    else
    {
        kputs("fatal: overlap on static vm node\n");
    }
}

void memory_init(struct kc_boot_data *boot_data)
{
    init_create_page_array(
            boot_data->phys_memory_map.base,
            boot_data->phys_memory_map.entries);

    void * object_space_head = (void *)page_align(
            boot_data->object_space.size +
            (uintptr_t)boot_data->object_space.base, 1);

    kputs("vm node 1\n");
    init_vm_node(
            &core_image_node,
            &kc_image_base,
            (void *)page_align(&kc_data_end, 1));
    kputs("vm node 2\n");
    init_vm_node(
            &core_object_node,
            (void *)page_align(&kc_data_end, 1),
            object_space_head);
    kputs("vm node 3\n");
    init_vm_node(&core_pagemaps_node, vm_temp, (void *)-1);
}

void *page_map(phys_addr_t paddr, enum page_map_flags flags)
{
    (void)paddr;
    (void)flags;
    return NULL;
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

    return (phys_addr_t)index * page_size(1);
}

void page_free(phys_addr_t paddr)
{
    int index = paddr / page_size(1);

    if ((size_t)index > page_array_entries)
    {
        return;
    }

    page_array[index].used = 0;
    page_array[index].next = first_free_page_index;
    first_free_page_index = index;
    free_pages++;
}

void *heap_alloc(size_t size)
{
    (void)size;
    return NULL;
}

void heap_free(void *block)
{
    (void)block;
}

void *memory_alloc(size_t size)
{
    (void)size;
    return NULL;
}

void memory_free(void *block)
{
    (void)block;
}

static uint64_t *get_kernel_pm1e(void *vaddr)
{
    return &kernel_pm1[kpm1_index(vaddr)];
}

static uint64_t *get_kernel_pm2e(void *vaddr)
{
    return &kernel_pm2[kpm2_index(vaddr)];
}

/*
   int anonymous_page_handler(uint32_t code, void *address)
   {
   kputs("anonymous space fault\n");
   if (code & PAGE_PR)
   {
// there's no reason a protection violation should happen
// in anonymous space
panic(UNHANDLED_FAULT);
}
// kernel page mappings are built different
if (address >= (void *)&kc_image_base)
{
uint64_t *pm2e = get_kernel_pm2e(address);

if (!page_address(*pm2e, 1))
{
// create a page table and install it
phys_addr_t pm1_phys = page_alloc();
if (pm1_phys)
{
uint64_t *pm1;
pm1 = page_map_at(temp, pm1_phys, CONTENT_RWDATA|SIZE_2M);
// zero the page
memset(pm1, 0, PAGE_SIZE);
// put the table where it goes
 *pm2e = page_address(pm1_phys, 1)|PAGE_WR|PAGE_PR;
 page_unmap(pm1);
 }
 else
 {
 panic(OUT_OF_MEMORY);
 }
 }

 uint64_t *pm1e = get_kernel_pm1e(address);

 if (pm1e)
 {
 phys_addr_t page_phys = page_alloc();
 if (page_phys)
 {
 *pm1e = page_address(page_phys, 1)|PAGE_WR|PAGE_PR;
// clear a potentially dirty page.
memset((void *)page_address(address, 1), 0, page_size(1));
}
}
}

return 0;
}
*/

int page_fault_handler(uint32_t code, void *address)
{
    (void)code;
    kputs("page fault\n");
    struct vm_tree_key key = {(uintptr_t)address, 1};
    struct vm_tree_node *node = vmt_search_key(&core_vm_tree, &key);
    if (node)
    {
        kputs("found memory object\n");
        int result = 1;
        if (result)
        {
            panic(UNHANDLED_FAULT);
        }
    }
    else
    {
        kputs("didn't find memory object\n");
        panic(UNHANDLED_FAULT);
    }
    return 0;
}

