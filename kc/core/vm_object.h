#pragma once

typedef int (*vm_object_handler_func)(uint32_t code, void *address);

enum vm_object_type
{
    NULL_VM_OBJECT, // a non-object. shouldn't even exist. an abomination unto man and god
    DIRECT_VM_OBJECT, // a 1:1 physical-to-virtual mapping
    TRANSLATION_VM_OBJECT, // a mapping between a physical and virtual address range
    ANONYMOUS_VM_OBJECT, // a mapping that has no definite physical location
    KC_IMAGE_VM_OBJECT, // a kernel component image
    KC_HEAP_VM_OBJECT, // a kernel component heap
    // TODO: more to come
};

struct vm_object
{
    enum vm_object_type type;
    vm_object_handler_func handler;
};

