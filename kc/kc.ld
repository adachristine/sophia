ENTRY(kc_entry)

SECTIONS
{
    PROVIDE_HIDDEN(kc_image_base = .);

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
        PROVIDE_HIDDEN(kc_text_begin = .);
        *(.text.start)
        *(.text)
        . = ALIGN(4K);
        PROVIDE_HIDDEN(kc_text_end = .);
    } =0xcc

    .dynamic : ALIGN(4K)
    {
        *(.dynamic)
    }

    .data :
    {
        PROVIDE_HIDDEN(kc_data_begin = .);
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
        . = ALIGN(4K);
        PROVIDE_HIDDEN(kc_data_end = .);
    }

    PROVIDE_HIDDEN(kc_image_end = .);

    /DISCARD/ :
    {
        *(.eh_frame)
        *(.comment)
    }
}

