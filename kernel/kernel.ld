ENTRY(kernel_entry)

PROVIDE(k_virt_base = 0xffffffff80000000);

SECTIONS
{
    .text k_virt_base : AT(ADDR(.text) - k_virt_base)
    {
        PROVIDE(k_text_begin = .);
        *(.text)
        . = ALIGN(4K);
        PROVIDE(k_text_end = .);
    } =0xcc

    .data ALIGN(4K) : AT(ADDR(.data) - k_virt_base)
    {
        PROVIDE(k_data_begin = .);
        *(.rodata)
        *(.data)
    }

    .bss :
    {
        *(.bss)
        PROVIDE(k_data_end = .);
    }

    /DISCARD/ :
    {
        *(.eh_frame)
    }
}

