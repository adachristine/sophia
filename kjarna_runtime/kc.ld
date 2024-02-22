ENTRY(_start)

SECTIONS
{
    PROVIDE_HIDDEN(kc_image_base = .);

    . = SIZEOF_HEADERS;

    .rela.plt :
    {
        *(.rela.plt)
    }

    .rela.dyn :
    {
        *(.rela.dyn)
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

    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    }

    .text : ALIGN(4K)
    {
        PROVIDE_HIDDEN(kc_text_begin = .);
        *(.text.start)
        *(.text)
    }

    .plt :
    {
        *(.plt)
        . = ALIGN(4K);
        PROVIDE_HIDDEN(kc_text_end = .);
    } =0xcc
    
    .dynamic : ALIGN(4K)
    {
        PROVIDE_HIDDEN(kc_data_begin = .);
        *(.dynamic)
    }

    .got :
    {
        *(.got)
    }

    .got.plt :
    {
        *(.got.plt)
    }

    .data :
    {
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
        *(.dynbss)
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

