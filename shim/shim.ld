ENTRY(efi_shim_entry)

SECTIONS
{
    . = sizeof_headers;

    .rel :
    {
        *(.rel.*)
    }

    .rela :
    {
        *(.rela.*)
    }

    .text : ALIGN(4K)
    {
        PROVIDE(_text_begin = .);
        *(.text)
        . = ALIGN(4K);
        PROVIDE(_text_end = .);
    } =0xcc

    .data :
    {
        PROVIDE(_data_begin = .);
        *(.rodata)
        *(.data)
    }

    .data.rel :
    {
        *(.data.rel)
        *(.data.rel.*)
    }

    .bss :
    {
        *(.bss)
        PROVIDE(_data_end = .);
    }

    /DISCARD/ :
    {
        *(.eh_frame)
    }
}

