#include <posix/unistd.h>
#include <posix/fcntl.h>
#include <kjarna/interface.h>
#include <libc/string.h>
#include <lib/elf.h>

#include "kjarna_efi.h"

#include "config.h"

#include <efi/error.h>

struct image_buffer
{
	char *base;
	size_t length;
};

static ssize_t read_ehdr(int fd, Elf64_Ehdr *ehdr)
{
	ssize_t result;

	if (efi_lseek(fd, 0, SEEK_SET) == -1)
	{
		return -1;
	}
	else if ((result = efi_read(fd, ehdr, sizeof(*ehdr))) == -1)
	{
		return -1;
	}
	else if (!elf64_validate(ehdr, ET_DYN, EM_X86_64))
	{
		return -1;
	}

	return result;
}

static ssize_t read_phdrs(int fd, Elf64_Ehdr *ehdr, Elf64_Phdr *phdrs)
{
	if (efi_lseek(fd, ehdr->e_phoff, SEEK_SET) != -1)
	{
		return efi_read(fd, phdrs, ehdr->e_phnum * ehdr->e_phentsize);
	}

	return -1;
}

static void create_image_buffer(Elf64_Ehdr *ehdr, Elf64_Phdr *phdrs, struct image_buffer *buffer)
{
	buffer->length = elf64_size(ehdr, phdrs);
	buffer->base = efi_mmap(nullptr, buffer->length, 0, 0, -1, 0);
}

static ssize_t load_segment(int fd, Elf64_Phdr *phdr, char *buffer)
{
	if (efi_lseek(fd, phdr->p_offset, SEEK_SET) == -1)
	{
		return -1;
	}

	int result = efi_read(fd, &buffer[phdr->p_offset], phdr->p_filesz);

	if (result > 0 && phdr->p_memsz > phdr->p_filesz)
	{
		memset(&buffer[phdr->p_offset + phdr->p_filesz], 0, phdr->p_memsz - phdr->p_filesz);
	}
	
	return result;
}

static ssize_t load_segments(int fd, Elf64_Ehdr *ehdr, Elf64_Phdr *phdrs, char *buffer)
{
	int i;

	for (i = 0; i < ehdr->e_phnum; i++)
	{
		if (phdrs[i].p_type != PT_LOAD)
		{
			continue;
		}

		if (load_segment(fd, &phdrs[i], buffer) == -1)
		{
			return -1;
		}
	}

	return i;
}

static struct image_buffer load_image(void)
{
	const char *image_entry_path = SERVICE_FILE_PATH;

	int image_fd;

	struct image_buffer buffer = { nullptr, 0 };

	if ((image_fd = efi_open(image_entry_path, O_RDONLY, 0)) == -1)
	{
		return buffer;
	}

	Elf64_Ehdr ehdr;

	if (read_ehdr(image_fd, &ehdr) == -1)
	{
		return buffer;
	}
	
	Elf64_Phdr *phdrs = efi_calloc_pool(ehdr.e_phnum, ehdr.e_phentsize);
	
	if (phdrs == nullptr)
	{
		return buffer;
	}
	else if(read_phdrs(image_fd, &ehdr, phdrs) != -1)
	{
		create_image_buffer(&ehdr, phdrs, &buffer);
	}

	if (buffer.base != nullptr && load_segments(image_fd, &ehdr, phdrs, buffer.base) == -1)
	{
		efi_munmap(buffer.base, buffer.length);
		buffer.base = nullptr;
	}

	efi_close(image_fd);
	efi_free_pool(phdrs);

	return buffer;
}

kjarna_image_entry_func *image_entry_addr(struct image_buffer *buffer)
{
	if (buffer->base == nullptr)
	{
		return nullptr;
	}

	Elf64_Ehdr *ehdr = (Elf64_Ehdr *)buffer->base;

	return (kjarna_image_entry_func *)(buffer->base + ehdr->e_entry);
}

int image_start(void)
{
	struct kjarna_interface interface =
	{
		efi_open,
		efi_close,
		efi_lseek,
		efi_read,
		efi_write,
		efi_mmap,
		efi_munmap
	};

	struct kjarna_entry_params params =
	{
		&interface,
		0,
		nullptr,
		nullptr
	};

	struct image_buffer buffer = load_image();

	kjarna_image_entry_func *image_entry = nullptr;

	if (buffer.base != nullptr)
	{
		image_entry = image_entry_addr(&buffer);
	}

	if (image_entry != nullptr)
	{
		image_entry(&params);
	}

	if (efi_munmap(buffer.base, buffer.length) == -1)
	{
		efi_write(STDOUT_FILENO, "failed unmap\r\n", -1);

		if (efi_errno == EFI_NOT_FOUND)
		{
			efi_write(STDOUT_FILENO, "not found\r\n", -1);
		}
		else
		{
			efi_write(STDOUT_FILENO, "bad alignment?\r\n", -1);
		}
	}

	return -1;
}

