#include "loader_efi.h"

static bool validate_image(Elf64_Ehdr *ehdr);
static size_t image_size(struct system_image *image);
static enum page_type get_page_type(Elf64_Phdr *phdr);

struct system_image *system_image_open(CHAR16 *path)
{
    EFI_STATUS status;
    EFI_FILE_PROTOCOL *root;
    EFI_FILE_PROTOCOL *file;
    struct system_image *image = NULL;

    status = e_system_partition->OpenVolume(e_system_partition, &root);

    if (EFI_ERROR(status))
    {
        Print(L"failed opening system partition root: %r\r\n", status);
        return NULL;
    }

    status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);

    if (EFI_ERROR(status))
    {
        Print(L"failed opening image file %s: %r\r\n", path, status);
        return NULL;
    }

    image = efi_allocate(sizeof(*image));

    if (!image)
    {
        Print(L"failed allocating image data: %r\r\n", e_last_error);
        goto failure;
    }

    image->file = file;

    UINTN ehdr_size = sizeof(image->ehdr);
    status = file->Read(file, &ehdr_size, &image->ehdr);

    if (EFI_ERROR(status))
    {
        Print(L"failed reading image file %s: %r\r\n", path, status);
        goto failure;
    }

    if (!validate_image(&image->ehdr))
    {
        Print(L"failed validating image file %s: %r\r\n", path, status);
        goto failure;
    }

    UINTN phdrs_size = image->ehdr.e_phentsize * image->ehdr.e_phnum;
    image->phdrs = efi_allocate(phdrs_size);

    if (!image->phdrs)
    {
        Print(L"failed allocating segment data: %r", e_last_error);
        goto failure;
    }

    status = file->Read(file, &phdrs_size, image->phdrs);

    if (EFI_ERROR(status))
    {
        Print(L"failed reading segment data: %r", status);
        goto failure;
    }

    return image;

failure:
    file->Close(file);
    return NULL;
}

bool system_image_load(struct system_image *image)
{
    image->buffer = system_allocate(image_size(image));

    if (image->buffer.type != SYSTEM_MEMORY)
    {
        return false;
    }

    e_bs->SetMem((VOID *)image->buffer.base, image->buffer.size, 0);

    Elf64_Ehdr *ehdr = &image->ehdr;
    Elf64_Phdr *phdrs = image->phdrs;

    Print(L"found %d segments in image\r\n", ehdr->e_phnum);

    EFI_STATUS status = EFI_SUCCESS;

    for (UINTN i = 0; i < ehdr->e_phnum && !EFI_ERROR(status); i++)
    {
        switch (phdrs[i].p_type)
        {
            case PT_LOAD:
                Print(L"loadable segment %16.0lx at offset %x of %d bytes\r\n",
                      phdrs[i].p_vaddr,
                      phdrs[i].p_offset,
                      phdrs[i].p_memsz);

                void *segment = (void *)(image->buffer.base + phdrs[i].p_paddr);
                UINTN segment_size = phdrs[i].p_filesz;

                image->file->SetPosition(image->file, phdrs[i].p_offset);
                status = image->file->Read(image->file, &segment_size, segment);
                break;
            default:
                continue;
        }
    }

    if (EFI_ERROR(status))
    {
        Print(L"failure reading segment from file: %r\r\n", status);
        return false;
    }

    return true;
}

void system_image_map(struct system_image *image)
{
    Elf64_Ehdr *ehdr = &image->ehdr;
    Elf64_Phdr *phdrs = image->phdrs;

    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdrs[i].p_type == PT_LOAD)
        {
            void *virt_begin = (void *)phdrs[i].p_vaddr;
            uint64_t phys_begin = image->buffer.base + phdrs[i].p_paddr;
            size_t size = phdrs[i].p_memsz;
            Print(L"mapping segment %16.0lx to %16.0lx %d bytes\r\n",
                  virt_begin,
                  phys_begin,
                  size);
            
            paging_map_pages(virt_begin,
                             phys_begin,
                             get_page_type(&phdrs[i]),
                             size);
        }
    }
}


static bool validate_image(Elf64_Ehdr *ehdr)
{
    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3 ||
        ehdr->e_ident[EI_CLASS] != ELFCLASS64 ||
        ehdr->e_ident[EI_DATA] != ELFDATA2LSB ||
        ehdr->e_ident[EI_VERSION] != EV_CURRENT ||
        ehdr->e_machine != EM_X86_64)
    {
        Print(L"invalid image format\r\n");
        return false;
    }

    Print(L"elf x64 image detected\r\n");

    return true;
}

static size_t image_size(struct system_image *image)
{
    size_t size = 0;
    Elf64_Phdr *phdrs = image->phdrs;

    for (int i = 0; i < image->ehdr.e_phnum; i++)
    {
        if (phdrs[i].p_type != PT_LOAD)
        {
            continue;
        }

        if (phdrs[i].p_paddr + phdrs[i].p_memsz > size)
        {
            size = phdrs[i].p_paddr + phdrs[i].p_memsz;
            if (phdrs[i].p_align > 1)
            {
                size = (size + phdrs[i].p_align - 1) & ~(phdrs[i].p_align - 1);
            }
        }
    }

    return size;
}

static enum page_type get_page_type(Elf64_Phdr *phdr)
{
    switch (phdr->p_flags)
    {
        case (PF_R|PF_X):
            return CODE_PAGE_TYPE;
        case (PF_R):
            return RODATA_PAGE_TYPE;
        case (PF_R|PF_W):
            return DATA_PAGE_TYPE;
    }

    return INVALID_PAGE_TYPE;
}
