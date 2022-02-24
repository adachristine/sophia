/* kernel virtual space guarantees
 * the loader must set up the address space as follows
 * 1. kernel virtual space is 2GiB in size on 2GiB alignment.
 * 2. the very top of address space must have a single self-mapped
 *    that manages the top 2MiB of space.
 * 3. that top 2MiB of space contains a bump-allocated buffer of some size
 *    recorded in the boot data structures
 */

#include "page_early.h"
#include "page_stack.h"

#include "memory.h"
#include "kprint.h"
#include "panic.h"
#include "vm_object.h"
#include "cpu/mmu.h"
#include "cpu/exceptions.h"

#include <stdint.h>

#include <kc.h>
#include <core/memory.h>

#define align_next(x, a) (x + a - 1) & ~(a - 1)

static void *temp_page_map(kc_phys_addr paddr, enum page_map_flags flags);
static void temp_page_unmap(void *address);

static void *page_map_at(
        void *vaddr,
        phys_addr_t paddr,
        enum page_map_flags flags);

enum vm_core_state_items
{
    KERNEL_VM_STATE,
    HEAP_VM_STATE,
    STACK_VM_STATE,
    TEMPS_VM_STATE
};

struct vm_core_static_state
{
    struct vm_tree_node node;
    struct vm_object *object;
};

static struct vm_core_state
{
    struct vm_tree tree;
    struct vm_object global_null;
    struct vm_object global_anonymous;
    struct vm_object global_direct;
    struct vm_object global_translate;
    struct vm_core_static_state statics[4];
    kc_phys_addr zero_page;
    void *first_free;
} vm_state = {
    {0},
    {NULL_VM_OBJECT, NULL},
    {ANONYMOUS_VM_OBJECT, anonymous_page_handler},
    {DIRECT_VM_OBJECT, NULL},
    {TRANSLATION_VM_OBJECT, NULL},
    {
        {{0}, &vm_state.global_null},
        {{0}, &vm_state.global_anonymous},
        {{0}, &vm_state.global_anonymous},
        {{0}, &vm_state.global_null},
    },
    0,
    NULL
};

static struct vm_temp_state
{
    uint64_t *table;
    int first_free;
}
temp_state;

struct vm_tree *vm_get_tree(void)
{
    return &vm_state.tree;
}

void page_init(void)
{
    kprintf("initializing page frame allocator\n");
    page_early_init();
    page_stack_init();
}

void page_init_final(void)
{
    kprintf("finishing page frame allocator initialization\n");
    page_early_final();
}


void page_set_present(kc_phys_addr page)
{
    page_stack_set_present(page);
}

int page_get_present(kc_phys_addr page)
{
    return page_stack_get_present(page);
}

void page_set_allocated(kc_phys_addr page)
{
    page_stack_set_allocated(page);
}

void page_set_free(kc_phys_addr page)
{
    page_stack_set_free(page);
}

kc_phys_addr page_alloc(enum page_alloc_flags type)
{
    kc_phys_addr paddr = 0;
    if ((paddr = page_stack_alloc(type)) > 0)
    {
        return paddr;
    }
    else
    {
        return page_early_alloc(type);
    }
}

void page_free(kc_phys_addr page)
{
    page_stack_free(page);
}

static void *map_tableset(void *vaddr, uint64_t *tables[4])
{
    uint64_t current_phys;
    uint64_t *current_pte;
    // pml4 is guaranteed to be present. no checks necessary.
    //

    current_phys = page_address(mmu_get_map(), 1);
    tables[3] = temp_page_map(current_phys, CONTENT_RWDATA);
    int n = 3;

    while (n)
    {
        if (tables[n])
        {
            current_pte = &tables[n][pte_index(vaddr, n+1)];

            if (!page_address(*current_pte, 1))
            {
                current_phys = page_alloc(PAGE_ALLOC_CONV);
                if (current_phys)
                {
                    tables[n-1] = temp_page_map(current_phys, CONTENT_RWDATA);
                    memset(tables[n-1], 0, page_size(1));
                    *current_pte = current_phys |= PAGE_NX|PAGE_WR|PAGE_PR;
                }
                else
                {
                    kprintf("error: failed allocating memory for page table\n");
                    PANIC(OUT_OF_MEMORY);
                }
            }
            else
            {
                current_phys = page_address(*current_pte, 1);
                tables[n-1] = temp_page_map(current_phys, CONTENT_RWDATA);
            }
        }
        n--;
    }

    if (tables[0])
    {
        current_pte = &tables[0][pte_index(vaddr, 1)];
    }

    return vaddr;
}

static void *page_map_at(
        void *vaddr,
        phys_addr_t paddr,
        enum page_map_flags flags)
{
    // TODO: add checks to prevent attempts to map reserved addreses
    //
    uint64_t *mapset[4] = {NULL};

    if (vaddr != map_tableset(vaddr, mapset))
    {
        PANIC(GENERAL_PANIC);
    }

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

    switch (flags & SIZE_MASK)
    {
        case SIZE_1G:
            offset = page_offset(paddr, 3);
            break;
        case SIZE_2M:
            offset = page_offset(paddr, 2);
            break;
        case 0:
        case SIZE_4K:
            offset = page_offset(paddr, 1);
            break;
        default:
            vaddr = NULL;
    }
    if (vaddr)
    {
        mapset[0][pte_index(vaddr, 1)] = page_address(paddr, 1) | entry;
    }

    for (int i = 0; i < 4; i++)
    {
        if (mapset[i])
        {
            temp_page_unmap(mapset[i]);
        }
    }

    return (char *)vaddr + offset;
}

#define VM_HEAP_SIZE 16 * page_size(2)

static void *temp_page_alloc(void)
{
    int index = temp_state.first_free;

    // do not allocate the 511th index!!
    if (index > -1 && index < 511)
    {
        temp_state.first_free = temp_state.table[index] >> 1;
        temp_state.table[index] = 0;
    }
    else
    {
        return NULL;
    }

    return (void *)-((512ULL - index) << 12);
}

static void temp_page_free(void *address)
{
    int index = pte_index(address, 1);

    if (index > -1 && index < 511)
    {
        temp_state.table[index] = temp_state.first_free << 1;
        temp_state.first_free = index;
    }
}

static void *temp_page_map(kc_phys_addr paddr, enum page_map_flags flags)
{
    uint64_t entry = PAGE_PR;
    size_t offset = 0;

    switch (flags & SIZE_MASK)
    {
        case 0:
        case SIZE_4K:
            offset = page_offset(paddr, 1);
            break;
        default:
            return NULL;
    }

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

    unsigned char *vaddr = temp_page_alloc();

    if (vaddr)
    {
        vaddr += offset;
        temp_state.table[pte_index(vaddr, 1)] = page_address(paddr, 1) | entry;
    }

    return vaddr;
}

static void temp_page_unmap(void *vaddr)
{
    temp_page_free(vaddr);
    mmu_invalidate(vaddr);
}

static void vm_init(void)
{
    kprintf("initializing vm state\n");
    struct kc_boot_data *boot_data = get_boot_data();
    struct vm_core_static_state *states = &vm_state.statics[0];

    // initialize the static vm node entries
    struct
    {
        unsigned char *base;
        unsigned char *head;
    } vm_ranges[] = {
        {&kc_image_base, &kc_image_end},
        {&kc_image_end, &kc_image_end + VM_HEAP_SIZE},
        {
            boot_data->buffer.base - page_size(1) * 2,
            boot_data->buffer.base - page_size(1)
        },
        {
            boot_data->buffer.current,
            boot_data->buffer.current + boot_data->buffer.max_size,
        }
    };

    vm_state.first_free = &kc_image_end;

    for (int i = 0; i <= TEMPS_VM_STATE; i++)
    {
        vmt_init_node
            (
             vm_get_tree(),
             &states[i].node,
             states[i].object,
             (void *)vm_ranges[i].base,
             (void *)vm_ranges[i].head
            );
    }

    // init the temporary mappings table state
    temp_state.table = (uint64_t *)-page_size(1);
    temp_state.first_free = -1;
    for (int index = 0; index < 512; index++)
    {
        // i love these array-based linked list stacks
        if (!(temp_state.table[index] & PAGE_PR))
        {
            temp_page_free((void *)-((512ULL - index) << 12));
        }
    }

    page_init();

    vm_state.zero_page = page_alloc(PAGE_ALLOC_CONV);
    if (!vm_state.zero_page)
    {
        kprintf("failed allocating for zero page\n");
        PANIC(GENERAL_PANIC);
    }

    void *zero_temp = temp_page_map(vm_state.zero_page, CONTENT_RWDATA);

    if (!zero_temp)
    {
        kprintf("failed mapping zero page for initialization\n");
        PANIC(GENERAL_PANIC);
    }

    // make the zero page live up to its name;
    memset(zero_temp, 0, page_size(1));
    temp_page_unmap(zero_temp);

    page_init_final();
}

void memory_init(void)
{
    vm_init();
}

void *page_map(phys_addr_t page, enum page_map_flags flags)
{
    void *vm_page = vm_alloc(4096, VM_ALLOC_TRANSLATE);
    if (vm_page)
    {
        page_map_at(vm_page, page, flags);
    }
    return NULL;
}

void page_unmap(void *vaddr)
{
    (void)vaddr;
}

struct heap_header
{
    size_t size;
    struct heap_header *next;
};

static struct heap_header *heap_root = (void *)-1ULL;

void *heap_alloc(size_t size)
{
    // simple first-fit allocator, allocates downward from the head
    // of the first block of sufficient size
    // TODO: join heap blocks if there is not one of sufficient size
    void *block = NULL;
    struct heap_header *header;

    // first attempt at allocation
    if ((void *)-1ULL == heap_root)
    {
        struct vm_tree_node *heap_node = &vm_state.statics[HEAP_VM_STATE].node;
        kprintf("initializing heap at %#lx of %zu bytes\n",
                heap_node->key.address, heap_node->key.size);
        heap_root = NULL;
        header = (struct heap_header *)heap_node->key.address;
        header->size = heap_node->key.size - sizeof(header->size);
        heap_free((char *)header + sizeof(*header));
    }
    else
    {
        header = heap_root;
    }
    size = align_next(size, sizeof(*header));

    while (header)
    {
        if (header->size > size)
        {
            break;
        }
        header = header->next;
    }

    if (header)
    {
        header->size -= size + sizeof(*header);
        header = (struct heap_header *)((char *)header + header->size);
        header->size = size;
        header->next = NULL;
        block = (char *)header + sizeof(*header);
    }

    return block;
}

void heap_free(void *block)
{
    struct heap_header *header = (void *)
        ((char *)block - sizeof(*header));

    header->next = heap_root;
    heap_root = header;
}

void *memory_alloc(size_t size)
{
    // allocations larger than page-size should just get an anonymous vm_object
    if (size < 4096)
    {
        return heap_alloc(size);
    }
    else
    {
        return vm_alloc(size, VM_ALLOC_ANY|VM_ALLOC_ANONYMOUS);
    }
}

void memory_free(void *block)
{
    // a little complicated to implement
    //
    // 1. find the vm_object that owns the block
    //   a. if the vm_object is a heap, call heap_free()
    //   b. if the vm_object is an anonymous vm_area, call vm_free().
    //   c. if the object is any other kind issue a bug warning and do nothing
    (void)block;
}

void *vm_alloc_at(void *address, size_t size, enum vm_alloc_flags flags)
{
    struct vm_tree_key key = {(uintptr_t)address, size};
    struct vm_tree_node *node;
    struct vm_object *object;

    switch (flags & VM_ALLOC_MECHANISM_MASK)
    {
        case VM_ALLOC_ANONYMOUS:
            object = &vm_state.global_anonymous;
            break;
        case VM_ALLOC_DIRECT:
            object = &vm_state.global_direct;
            break;
        case VM_ALLOC_TRANSLATE:
            object = &vm_state.global_translate;
            break;
        default:
            return NULL;
    }

    if (!vmt_search_key(vm_get_tree(), &key) &&
            (node = heap_alloc(sizeof(*node))))
    {
        vmt_init_node(
                vm_get_tree(),
                node,
                object,
                address,
                (void *)((char *)address + size));
    }
    
    return address;
}

void *vm_alloc(size_t size, enum vm_alloc_flags flags)
{
    (void)flags;
    // TODO implement a proper allocator here rather than this bump allocator.
    char *address = vm_state.first_free;

    if (address == vm_alloc_at(address, size, flags))
    {
        vm_state.first_free = address + size;
        return address;
    }

    return NULL;
}

int anonymous_page_handler(
        struct vm_tree_node *node,
        uint32_t code,
        void *address)
{
    (void)node;

    if (!(code & 1))
    {
        // map the zero page read-only to the address
        page_map_at(
                address,
                vm_state.zero_page,
                CONTENT_RODATA|SIZE_4K);
    }

    if ((code & 1) && (code & 2)) // page fault write violation on present page
    {
        mmu_invalidate(address);
        kc_phys_addr paddr = page_alloc(PAGE_ALLOC_CONV);

        if (!paddr)
        {
            kprintf("got zero from page_alloc :|\n");
            PANIC(OUT_OF_MEMORY);
        }
        page_map_at(
                address,
                page_alloc(PAGE_ALLOC_CONV),
                CONTENT_RWDATA|SIZE_4K);
        // NULL out the whole page
        // TODO: thread to clean dirty pages.
        memset((void *)page_address(address, 1), 0, page_size(1));
    }

    return 0;
}

void page_fault_handler(struct isr_context *context)
{
    void *address;
    __asm__ volatile ("movq %%cr2, %0" : "=r"(address));

    struct vm_tree_key key = {(uintptr_t)address, sizeof(uint64_t)};
    struct vm_tree_node *node = vmt_search_key(vm_get_tree(), &key);

    if (!node)
    {
        print_exception_context(context);
        kprintf("error: page fault in unmanaged address %#lx\n", address);
        PANIC(UNHANDLED_FAULT);
    }

    if (!node->object->handler)
    {
        print_exception_context(context);
        kprintf("error: vm object at %p has no fault handler\n");
        PANIC(UNHANDLED_FAULT);
    }
}

void general_protection_handler(struct isr_context *context)
{
    //TODO: implement #gp handler
    kputs("general protection violation\n");
    print_exception_context(context);
    PANIC(UNHANDLED_FAULT);
}

