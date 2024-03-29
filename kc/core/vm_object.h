#pragma once

#include "vm_tree.h"

typedef int (*vm_object_handler_func)(
        struct vm_tree_node *node,
        uint32_t code,
        void *address);

enum vm_object_type
{
    NULL_VM_OBJECT, // a non-object. shouldn't even exist. an abomination unto man and god
    DIRECT_VM_OBJECT, // a 1:1 physical-to-virtual mapping
    TRANSLATION_VM_OBJECT, // a mapping between a physical and virtual address range
    ANONYMOUS_VM_OBJECT, // a mapping that has no definite physical location
    // TODO: more to come
};

struct vm_object
{
    enum vm_object_type type;
    vm_object_handler_func handler;
};

