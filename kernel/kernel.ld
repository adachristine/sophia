ENTRY(kernel_entry)

k_virt_base = 0xffffffff80000000;

SECTIONS
{
    .text k_virt_base : AT(ADDR(.text) - k_virt_base)
    {
        *(.text)
        . = ALIGN(4K);
    } =0x90

    .data ALIGN(4K) : AT(ADDR(.data) - k_virt_base)
    {
        *(.rodata)
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }

    /DISCARD/ :
    {
        *(.eh_frame)
    }
}

