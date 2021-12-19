ENTRY(kc_entry)

SECTIONS
{
    PROVIDE(kc_image_base = .);

    . = sizeof_headers;

    .rel :
    {
        *(.rel)
        *(.rel.*)
    }

    .rela :
    {
        *(.rel)
        *(.rela.*)
    }

    .hash :
    {
        *(.hash)
    }
    
    .dynsym :
    {
        *(.dynsym)
    }

    .dynstr :
    {
        *(.dynstr)
    }

    .text : ALIGN(4K)
    {
        PROVIDE(kc_text_begin = .);
        *(.text.start)
        *(.text)
        . = ALIGN(4K);
        PROVIDE(kc_text_end = .);
    } =0xcc

    .dynamic : ALIGN(4K)
    {
        *(.dynamic)
    }

    .data :
    {
        PROVIDE(kc_data_begin = .);
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
        PROVIDE(kc_data_end = .);
    }

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
    }
}

