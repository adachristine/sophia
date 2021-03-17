#include <stdbool.h>
#include <stdnoreturn.h>

#include <boot/entry/entry_efi.h>

static const char kernel_entry_data[4096];

noreturn void kernel_entry(struct efi_boot_data *data)
{
    (void)data;
    __asm__ ("cli\n\t");
    while (true) __asm__ ("hlt\n\t");
}

