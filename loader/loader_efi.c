#include "loader_efi.h"

#include <loader/data_efi.h>

#include <kernel/entry.h>

static CHAR16 *kernel_path = L"\\adasoft\\sophia\\kernel.os";
static struct system_image *kernel_image = NULL;

static struct efi_memory_map_data *get_memory_map_data(void);
static struct efi_framebuffer_data *get_framebuffer_data(void);
static void *get_acpi_rsdp(void);

noreturn static void enter_kernel()
{
    struct efi_boot_data data;
    kernel_entry_func kernel_entry;
    kernel_entry = (kernel_entry_func)(kernel_image->ehdr.e_entry);

    struct memory_range kernel_entry_stack =
        system_allocate(KERNEL_ENTRY_STACK_SIZE);

    // get a stack here for now idfk
    paging_map_range(KERNEL_ENTRY_STACK_BASE,
                     &kernel_entry_stack,
                     DATA_PAGE_TYPE);

    data.system_table = e_st;
    data.framebuffer = get_framebuffer_data();
    data.acpi_rsdp = get_acpi_rsdp();
    data.memory_map = get_memory_map_data();

    EFI_STATUS status = e_bs->ExitBootServices(e_image_handle,
                                               data.memory_map->key);

    if (EFI_ERROR(status))
    {
        Print(L"failed exiting boot services environment: %r\r\n", status);
        efi_exit(status);
    }

    // enter the kernel address environment
    paging_enter();
    
    // set the kernel entry stack.
    __asm__
    (
        "mov %0, %%rsp"
        :
        : "r"((uint64_t)KERNEL_ENTRY_STACK_HEAD)
    );

    kernel_entry(&data);
    
    while(true);
}

void loader_main(void)
{
    kernel_image = system_image_open(kernel_path);

    Print(L"loading %s\r\n", kernel_path);

    if (system_image_load(kernel_image))
    {
        Print(L"loaded kernel image at base %16.0lx size %d bytes\r\n",
              kernel_image->buffer.base,
              kernel_image->buffer.size);
    }
    else
    {
        Print(L"failed loading kernel image\r\n");
        efi_exit(EFI_ABORTED);
    }
    
    paging_init();
    system_image_map(kernel_image);
    Print(L"kernel maps created. entering kernel\r\n");
    enter_kernel();

    efi_exit(EFI_ABORTED);
}

static struct efi_memory_map_data *get_memory_map_data(void)
{
    EFI_STATUS status;
    UINTN mapsize = 0;
    struct efi_memory_map_data *map = NULL;

    status = e_bs->GetMemoryMap(&mapsize, NULL, NULL, NULL, NULL);

    if (status == EFI_BUFFER_TOO_SMALL)
    {
        map = efi_allocate(sizeof(*map) + mapsize);
    }

    if (map)
    {
        status = e_bs->GetMemoryMap(&map->size,
                                    map->data,
                                    &map->key,
                                    &map->descsize,
                                    &map->descver);
    }
    else
    {
        status = e_last_error;
    }
     
    if (EFI_ERROR(status))
    {
        Print(L"failed getting memory map: %r\r\n", status);
        return NULL;
    }

    return map;
}

static struct efi_framebuffer_data *get_framebuffer_data(void)
{
    struct efi_framebuffer_data *framebuffer;

    if (!e_graphics_output)
    {
        return NULL;
    }

    framebuffer = efi_allocate(sizeof(*framebuffer));

    if (framebuffer)
    {
        EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE *mode = e_graphics_output->Mode;

        framebuffer->bitmask = mode->Info->PixelInformation;
        framebuffer->width = mode->Info->HorizontalResolution;
        framebuffer->height = mode->Info->VerticalResolution;
        framebuffer->buffer.base = mode->FrameBufferBase;
        framebuffer->buffer.size = mode->FrameBufferSize;
        framebuffer->buffer.type = RESERVED_MEMORY;
        framebuffer->pxformat = mode->Info->PixelFormat;

        switch (framebuffer->pxformat)
        {
            //falthrough
            case PixelBlueGreenRedReserved8BitPerColor:
            case PixelRedGreenBlueReserved8BitPerColor:
                framebuffer->pxsize = sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
                break;
            default:
                // TODO: calculate pixel size for bitmask format
                framebuffer->pxsize = 0;
        }

        framebuffer->pitch = framebuffer->width * framebuffer->pxsize;
    }
    else
    {
        Print(L"failed getting framebuffer data: %r\r\n", e_last_error);
        return NULL;
    }

    return framebuffer;
}

static void *get_acpi_rsdp(void)
{
    EFI_GUID acpi_20_guid = ACPI_20_TABLE_GUID;

    for (UINTN i = 0; i < e_st->NumberOfTableEntries; i++)
    {
        if (!CompareGuid(&acpi_20_guid, &e_st->ConfigurationTable[i].VendorGuid))
        {
            Print(L"acpi rsdp: 0x%16.0lx\r\n",
                  e_st->ConfigurationTable[i].VendorTable);
            
            return e_st->ConfigurationTable[i].VendorTable;
        }
    }

    Print(L"failed getting acpi rsdp\r\n");
    return NULL;
}

