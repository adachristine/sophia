ENTRY(kernel_entry)

SECTIONS
{
    . = SIZEOF_HEADERS;

    .text 0 :
    {
        *(.text)
    } =0x90

    .data : ALIGN(4K)
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

