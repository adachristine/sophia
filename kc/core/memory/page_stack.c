#include "page_stack.h"
#include "vm_tree.h"
#include "vm_object.h"

#include <kc.h>

#define PAGE_STACK_CACHE_SIZE (1ULL << 32)
#define page_stack_index(x) (x / page_size(1))
#define page_stack_address(x) (x * page_size(1))

struct page
{
    uint32_t present: 1; // a physical page is at this location
    uint32_t allocated: 1; // this physical page has been taken
    int32_t next_refs; // allocated = 0: the index of the next page in the list
                       // allocated = 1: the number of references this page has
};

static struct page_stack_state
{
    struct vm_tree_node node;
    struct vm_object object;
    struct page *stack;
    int32_t free_count[3];
    int32_t total_count[3];
    int32_t first_free[3];
}
stack_state = {
    {0},
    {ANONYMOUS_VM_OBJECT, anonymous_page_handler},
    NULL,
    {0,0,0},
    {0,0,0},
    {-1,-1,-1}
};

static enum page_alloc_flags stack_type(kc_phys_addr page);
static void stack_push(kc_phys_addr page);
static kc_phys_addr stack_pop(enum page_alloc_flags type);

void page_stack_init(void)
{   
    vmt_init_node(
            vm_get_tree(),
            &stack_state.node,
            &stack_state.object,
            &kc_image_base - PAGE_STACK_CACHE_SIZE,
            &kc_image_base);

    stack_state.stack = (struct page *)stack_state.node.key.address;
}

kc_phys_addr page_stack_alloc(enum page_alloc_flags type)
{
    kc_phys_addr page = stack_pop(type);
    if (page)
    {
        page_stack_inc_ref(page);
    }
    return page;
}

void page_stack_free(kc_phys_addr page)
{

    // freeing a page only makes sense for present physical pages
    if (!page_stack_get_present(page))
    {
        return;
    }

    // immediately release a reference
    int referents = page_stack_dec_ref(page);

    // only actually free the page if it has no more references
    if (referents == 0)
    {
        page_stack_set_free(page);
        stack_push(page);
    }
}

int page_stack_get_present(kc_phys_addr page)
{
    unsigned index = page_stack_index(page);
    return stack_state.stack[index].present;
}

void page_stack_set_present(kc_phys_addr page)
{
    unsigned index = page_stack_index(page);
    stack_state.stack[index].present = 1;
}

void page_stack_set_allocated(kc_phys_addr page)
{
    unsigned index = page_stack_index(page);
    stack_state.stack[index].allocated = 1;
    stack_state.stack[index].next_refs = 1;
}

void page_stack_set_free(kc_phys_addr page)
{
    unsigned index = page_stack_index(page);
    stack_state.stack[index].allocated = 0;
    stack_state.stack[index].next_refs = -1;
}

int page_stack_inc_ref(kc_phys_addr page)
{
    unsigned index = page_stack_index(page);
    // taking a reference only makes sense for an allocated page
    // and a page that's actually physical
    if (stack_state.stack[index].allocated)
    {
        return ++stack_state.stack[index].next_refs;
    }

    return -1;
}

int page_stack_dec_ref(kc_phys_addr page)
{   
    unsigned index = page_stack_index(page);
    // releasing a reference only makes sense for an allocated page
    if (stack_state.stack[index].allocated)
    {
        // releasing a reference only makes sense if the refcount is > 0
        if (stack_state.stack[index].next_refs > 0)
        {
            stack_state.stack[index].next_refs--;
        }
        return stack_state.stack[index].next_refs;
    }

    return -1;
}

static enum page_alloc_flags stack_type(kc_phys_addr page)
{
    // the page stack type only makes sense for physical pages
    if (!page_stack_get_present(page))
    {
        return PAGE_ALLOC_NONE;
    }

    // pages above 4GiB physical are high memory
    if (page > -1U)
    {
        return PAGE_ALLOC_HIGH;
    }

    // pages below 1MiB are low memory
    else if (page < 0x100000)
    {
        return PAGE_ALLOC_LOW;
    }
    
    // all other pages are conventional memory
    return PAGE_ALLOC_CONV;
}

static void stack_push(kc_phys_addr page)
{
    enum page_alloc_flags type = stack_type(page);

    // stack push only makes sense for pages in a physical stack
    if ((type < PAGE_ALLOC_LOW) || type > PAGE_ALLOC_HIGH)
    {
        return;
    }

    unsigned index = page_stack_index(page);
    stack_state.stack[index].next_refs = stack_state.first_free[type - 1];
    stack_state.first_free[type - 1] = index;
    stack_state.free_count[type - 1]++;
}

static kc_phys_addr stack_pop(enum page_alloc_flags type)
{   
    int pop_stack_index = -1;
    int pop_stack_last = -1;

    // pops only make sense for pages with a stack type
    if ((type < PAGE_ALLOC_LOW) || (type > PAGE_ALLOC_HIGH))
    {
        return 0;
    }

    // do allocations in a high-to-low order if we're allocating any
    if (type == PAGE_ALLOC_ANY)
    {
        pop_stack_index = PAGE_ALLOC_HIGH;
        pop_stack_last = PAGE_ALLOC_LOW;
    }
    else
    {
        pop_stack_index = type;
        pop_stack_last = type;
    }

    for (; pop_stack_last <= pop_stack_index; --pop_stack_index)
    {
        if (stack_state.first_free[pop_stack_index - 1] > 0)
        {
            unsigned index = stack_state.first_free[pop_stack_index-1];
            stack_state.first_free[pop_stack_index-1] = 
                stack_state.stack[index].next_refs;
            page_stack_set_allocated(page_stack_address(index));
            stack_state.free_count[pop_stack_index - 1]--;
            return page_stack_address(index);
        }
    }

    return 0;
}

