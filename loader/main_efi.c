#include <loader/efi/shim.h>

#include <kernel/entry.h>

#include <efi/types.h>
#include <efi/error.h>

#include "elf.h"
#include "kstdio.h"

FILE *kstdout;
FILE *kstderr;

static EFI_SYSTEM_TABLE *gST;
static EFI_BOOT_SERVICES *gBS;

EFI_STATUS alloc_page(EFI_MEMORY_TYPE type,
        UINTN size,
        EFI_PHYSICAL_ADDRESS *base);
EFI_STATUS free_page(EFI_PHYSICAL_ADDRESS base, UINTN size);

EFI_STATUS open_image(struct efi_loader_image *image);
EFI_STATUS allocate_image(struct efi_loader_image *image);
EFI_STATUS load_image(struct efi_loader_image *image);

static EFI_STATUS enter_shim(void);

static const CHAR16 *efi_strerror(EFI_STATUS s);

static struct efi_loader_image shim_image =
{   
    .path = L"\\adasoft\\sophia\\efi.os"
};

static CHAR16 *linebuf;
static CHAR16 *linebuf_ptr;

static VOID *AllocatePool(UINTN size)
{
    EFI_STATUS status;
    void *block = NULL;

    if (gBS)
    {
        status = gBS->AllocatePool(
                EfiLoaderData,
                size,
                &block);
    }
    else
    {
        status = EFI_NOT_READY;
    }

    if (!EFI_ERROR(status))
    {
        return block;
    }

    return NULL;
}

static VOID FreePool(void *block)
{
    if (gBS)
    {
        gBS->FreePool(block);
    }
}

static struct efi_loader_interface loader_interface;

static const CHAR16 *efi_status_strings[] =
{
    L"operation successful", // 0
    L"image failed to load", // 1
    L"invalid parameter", // 2
    L"operation not supported", // 3
    L"wrongly-sized buffer", // 4
    L"buffer too small", // 5
    L"not ready", // 6
    L"device error", // 7
    L"attempt to write to write-protected device", // 8
    L"out of resoures", // 9
    L"storage volume corrupted", // 10
    L"storage volume full", // 11
    L"no storage media", // 12
    L"storage media changed", // 13
    L"item not found", // 14
    L"access denied", // 15
    L"no response", // 16
    L"no mapping",
    L"device timed out",
    L"protocol already started",
    L"operation aborted",
    L"ICMP error",
    L"TFTP error",
    L"network protocol error",
    L"incompatible version",
    L"security violation",
    L"CRC32 checksum error",
    L"end of storage media",
    L"end of file",
    L"invalid language",
    L"security compromosed",
    L"HTTP error"
};

/**
static const CHAR16 *efi_warning_strings[] =
{
    L"operation successful",
    L"warning: unknown glyph",
    L"warning: file not deleted",
    L"warning: file write failure",
    L"warning: buffer too small",
    L"warning: stale data",
    L"warning: contains filesystem",
    L"warning: system reset required"
};
**/

int kfputc(int c, FILE *f)
{
    (void)f;
    EFI_STATUS status = EFI_SUCCESS;

    if (loader_interface.system_table)
    {
        EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *out = gST->ConOut;
        *linebuf_ptr++ = c;
        if (((linebuf_ptr - linebuf) == EFI_PAGE_SIZE - 2) || (c == L'\n'))
        {
            *linebuf_ptr++ = 0;
            status = out->OutputString(out, linebuf);
            linebuf_ptr = linebuf;
        }
    }
    else
    {
        return -1;
    }

    if (EFI_ERROR(status))
    {
        return -1;
    }

    return c;
}

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    loader_interface = (struct efi_loader_interface){
        image_handle,
        system_table,
        alloc_page,
        free_page,
        open_image,
        allocate_image,
        load_image
    };

    gST = system_table;
    gBS = system_table->BootServices;

    EFI_STATUS status;

    if EFI_ERROR((status = alloc_page(
                    EfiLoaderData, 
                    EFI_PAGE_SIZE, 
                    (EFI_PHYSICAL_ADDRESS *)&linebuf)))
    {
        gST->ConOut->OutputString(
                gST->ConOut,
                L"error allocating for line buffer");
        return status;
    }
    else
    {
        gST->ConOut->Reset(gST->ConOut, FALSE);
        gBS->SetMem(linebuf, EFI_PAGE_SIZE, 0);
        linebuf_ptr = linebuf;
    }

    kprintf("loader interface %p\r\n"
            "image_handle %p\r\n"
            "system_table %p\r\n"
            "page_alloc %p\r\n"
            "page_free %p\r\n"
            "image_open %p\r\n"
            "image_alloc %p\r\n"
            "image_load %p\r\n",
            &loader_interface,
            loader_interface.image_handle,
            loader_interface.system_table,
            loader_interface.page_alloc,
            loader_interface.page_free,
            loader_interface.image_open,
            loader_interface.image_alloc,
            loader_interface.image_load);

    if (EFI_ERROR((status = open_image(&shim_image))))
    {
        kprintf("error opening shim image: %ls\r\n", 
                efi_strerror(status));
    }

    if (EFI_ERROR((status = allocate_image(&shim_image))))
    {
        kprintf("error allocating shim image: %ls\r\n",
                efi_strerror(status));
    }

    if (EFI_ERROR((status = load_image(&shim_image))))
    {
        kprintf("error loading shim image: %ls\r\n",
                efi_strerror(status));
    }

    if (EFI_ERROR((status = enter_shim())))
    {
        kprintf("error running shim image: %ls\r\n",
                efi_strerror(status));
    }

    if (shim_image.root)
    {
        shim_image.root->Close(shim_image.root);
    }
    if (shim_image.file)
    {
        shim_image.file->Close(shim_image.file);
    }

    if (shim_image.buffer_base && shim_image.buffer_size)
    {
        free_page(shim_image.buffer_base,
                EFI_SIZE_TO_PAGES(shim_image.buffer_size));
    }

    return status;
}

EFI_STATUS alloc_page(EFI_MEMORY_TYPE type,
        UINTN size,
        EFI_PHYSICAL_ADDRESS *base)
{
    return gBS->AllocatePages(AllocateAnyPages,
            type,
            EFI_SIZE_TO_PAGES(size),
            base);
}

EFI_STATUS free_page(EFI_PHYSICAL_ADDRESS base, UINTN size)
{
    return gBS->FreePages(base, EFI_SIZE_TO_PAGES(size));
}


EFI_STATUS open_image(struct efi_loader_image *image)
{
    EFI_FILE_PROTOCOL *root;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *root_fs;
    EFI_GUID root_fs_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

    EFI_LOADED_IMAGE_PROTOCOL *loader_image;
    EFI_GUID loader_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;

    EFI_STATUS status;

    status = gBS->OpenProtocol(
            loader_interface.image_handle,
            &loader_image_guid,
            (void  *)&loader_image,
            loader_interface.image_handle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    if (EFI_ERROR(status))
    {
        kprintf(
                "failed locating image protocol: %ls\r\n", 
                efi_strerror(status));
        return status;
    }

    status = gBS->OpenProtocol(
            loader_image->DeviceHandle,
            &root_fs_guid,
            (void *)&root_fs,
            loader_interface.image_handle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL);

    if (EFI_ERROR(status))
    {
        kprintf(
                "failed locating ESP file system: %ls\r\n", 
                efi_strerror(status));
        return status;
    }

    status = root_fs->OpenVolume(root_fs, &root);

    if (EFI_ERROR(status))
    {
        kprintf(
                "failed opening root device: %ls\r\n", 
                efi_strerror(status));
        return status;
    }

    kprintf("opening image %ls\r\n", image->path);

    status = root->Open(root,
            &image->file,
            image->path,
            EFI_FILE_MODE_READ,
            0);

    if (!EFI_ERROR(status))
    {
        image->root = root;
    }
    else
    {
        kprintf("failed opening image %ls: %r\r\n", image->path, status);
        return status;
    }

    Elf64_Ehdr ehdr;
    UINTN ehdr_size = sizeof(ehdr);

    Elf64_Phdr *phdrs = NULL;
    UINTN phdrs_size = 0;

    if (!EFI_ERROR((status = image->file->Read(image->file,
                        &ehdr_size, 
                        &ehdr))))
    {
        if (elf_validate(&ehdr, ET_EXEC, EM_X86_64) ||
                elf_validate(&ehdr, ET_DYN, EM_X86_64))
        {
            phdrs_size = ehdr.e_phentsize * ehdr.e_phnum;
            phdrs = AllocatePool(phdrs_size);
         }
         else
         {
            FreePool(phdrs);
            kprintf("invalid image format detected\r\n");
            return EFI_INVALID_PARAMETER;
         }
    }
    else
    {
        kprintf("failed reading shim image headers\r\n");
        return status;
    }

    if (!EFI_ERROR((status = image->file->Read(image->file,
                        &phdrs_size,
                        phdrs))))
    {
        image->buffer_size = elf_size(&ehdr,  phdrs);
        image->system_table = gST;
        kprintf("elf image %d bytes\r\n", image->buffer_size);
    }
    else
    {
        kprintf("failed reading shim image segments\r\n");
    }

    FreePool(phdrs);

    return status;
}

EFI_STATUS allocate_image(struct efi_loader_image *image)
{
    kprintf("allocating image %ls\r\n", image->path);
    EFI_STATUS status;

    status = alloc_page(SystemMemoryType,
            image->buffer_size,
            &image->buffer_base);

    if (!EFI_ERROR(status))
    {
        gBS->SetMem((void *)image->buffer_base, image->buffer_size, 0);
    }

    return status;
}

static EFI_STATUS read_image(struct efi_loader_image *image, 
        VOID **buffer,
        UINTN offset,
        UINTN length)
{
    kprintf("reading %d bytes from image at %p %ls\r\n",
        length,
        offset,
        image->path);

    EFI_STATUS status;

    if (!EFI_ERROR((status = image->file->SetPosition(image->file, offset))))
    {
        *buffer = (void *)(image->buffer_base + offset);
        status = image->file->Read(image->file, &length, *buffer);
    }
    else
    {
        kprintf("failed seeking shim image: %ls\r\n", efi_strerror(status));
    } 

    return status;
}

EFI_STATUS load_image(struct efi_loader_image *image)
{
    EFI_STATUS status;
    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdrs;

    if (!EFI_ERROR((status = read_image(image,
                        (void **)&ehdr,
                        0,
                        sizeof(*ehdr)))) &&
            !EFI_ERROR((status = read_image(image,
                        (void **)&phdrs,
                        ehdr->e_phoff,
                        ehdr->e_phnum * ehdr->e_phentsize))))
    {
        for (UINTN i = 0; i < ehdr->e_phnum && !EFI_ERROR(status); i++)
        {
            if (PT_LOAD != phdrs[i].p_type)
            {
                continue;
            }

            void *placement;
            status = read_image(image,
                    &placement,
                    phdrs[i].p_offset,
                    phdrs[i].p_filesz);

            if (!EFI_ERROR(status))
            {
                kprintf("loaded segment %p of %d bytes "
                        "(file size %d bytes)\r\n",
                        placement,
                        phdrs[i].p_memsz,
                        phdrs[i].p_filesz);
            }
        }
    }

    return status;
}

static EFI_STATUS enter_shim(void)
{
    EFI_STATUS status = EFI_ABORTED;
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)shim_image.buffer_base;
    efi_shim_entry_func entry;

    entry = (efi_shim_entry_func)(shim_image.buffer_base + ehdr->e_entry);
    kprintf("entering shim @%p\r\n", entry);

    status = entry(&loader_interface);

    return status;
}

static const CHAR16 *efi_strerror(EFI_STATUS s)
{
    return efi_status_strings[EFI_ERROR(s)];
}

