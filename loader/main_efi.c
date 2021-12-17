#include <loader/efi/shim.h>

#include <efi.h>
#include <efidebug.h>
#include <efilib.h>

#include "../lib/elf.h"

EFI_STATUS alloc_page(EFI_MEMORY_TYPE type,
        UINTN size,
        EFI_PHYSICAL_ADDRESS *base);
EFI_STATUS free_page(EFI_PHYSICAL_ADDRESS base, UINTN size);

EFI_STATUS open_image(struct efi_loader_image *image);
EFI_STATUS allocate_image(struct efi_loader_image *image);
EFI_STATUS load_image(struct efi_loader_image *image);

static EFI_STATUS enter_shim(void);

static struct efi_loader_image shim_image =
{   
    .path = L"\\adasoft\\sophia\\efi.os"
};

static struct efi_loader_interface loader_interface =
{
    &alloc_page,
    &free_page,

    &open_image,
    &allocate_image,
    &load_image
};

EFI_STATUS efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *system_table)
{
    InitializeLib(image_handle, system_table);

    shim_image.image_handle = image_handle;

    EFI_STATUS status;

    if (EFI_ERROR((status = open_image(&shim_image))))
    {
        Print(L"error opening shim image: %r\r\n");
    }

    if (EFI_ERROR((status = allocate_image(&shim_image))))
    {
        Print(L"error allocating shim image: %r\r\n");
    }

    if (EFI_ERROR((status = load_image(&shim_image))))
    {
        Print(L"error loading shim image: %r\r\n");
    }

    if (EFI_ERROR((status = enter_shim())))
    {
        Print(L"error running shim image: %r\r\n");
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
    return gBS->FreePages(base, size);
}


EFI_STATUS open_image(struct efi_loader_image *image)
{
    EFI_FILE_PROTOCOL *root;

    EFI_LOADED_IMAGE_PROTOCOL *loader_image;

    EFI_STATUS status;

    status = gBS->OpenProtocol(image->image_handle,
            &LoadedImageProtocol,
            (void  *)&loader_image,
            image->image_handle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL);


    if (EFI_ERROR(status) ||
            !(root = LibOpenRoot(loader_image->DeviceHandle)))
    {
        Print(L"failed opening root device\r\n");
        return EFI_NOT_FOUND;
    }

    Print(L"opening image %s\r\n", image->path);

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
        Print(L"failed opening image: %r\r\n", status);
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
            ASSERT(phdrs);
         }
         else
         {
            FreePool(phdrs);
            Print(L"invalid image format detected\r\n");
            return EFI_INVALID_PARAMETER;
         }
    }
    else
    {
        Print(L"failed reading shim image headers\r\n");
        return status;
    }

    if (!EFI_ERROR((status = image->file->Read(image->file,
                        &phdrs_size,
                        phdrs))))
    {
        image->buffer_size = elf_size(&ehdr,  phdrs);
        image->system_table = gST;
        Print(L"elf image %d bytes\r\n", image->buffer_size);
    }
    else
    {
        Print(L"failed reading shim image segments\r\n");
    }

    FreePool(phdrs);

    return status;
}

EFI_STATUS allocate_image(struct efi_loader_image *image)
{
    Print(L"allocating image %s\r\n", image->path);
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
    Print(L"reading %d bytes from image at 0x%8.0x %s\r\n", length, offset, image->path);
    EFI_STATUS status;

    ASSERT(buffer);

    if (!EFI_ERROR((status = image->file->SetPosition(image->file, offset))))
    {
        *buffer = (void *)(image->buffer_base + offset);
        status = image->file->Read(image->file, &length, *buffer);
    }
    else
    {
        Print(L"failed seeking shim image\r\n");
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
                Print(L"loaded segment 0x%16.0x of %d bytes "
                        L"(file size %d bytes)\r\n",
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
    EFI_STATUS status;
    Elf64_Ehdr *ehdr = (Elf64_Ehdr *)shim_image.buffer_base;
    efi_shim_entry_func entry;

    entry = (efi_shim_entry_func)(shim_image.buffer_base + ehdr->e_entry);
    status = entry(&shim_image, &loader_interface);
    return status;
}

