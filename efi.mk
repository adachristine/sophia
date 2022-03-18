# vim: ft=make:nowrap:ts=4:sw=4

ifeq ($(ARCH),x86_64)
 GNUEFIARCH := x86_64
endif
ifeq ($(ARCH),i686)
 GNUEFIARCH := ia32
endif

ifndef EFIAPIDIR
EFIAPIDIR := /usr/local/include/efi
endif

ifndef EFILIBDIR
EFILIBDIR := /usr/local/$(TARGET)/lib
endif

EFICRT := $(EFILIBDIR)/crt0-efi-$(GNUEFIARCH).o
EFILDSCRIPT := $(EFILIBDIR)/elf_$(GNUEFIARCH)_efi.lds

CPPFLAGS += -I$(EFIAPIDIR) -I$(EFIAPIDIR)/protocol -I$(EFIAPIDIR)/$(GNUEFIARCH) \
            -DGNU_EFI_USE_MS_ABI -DEFI_CALL_WRAPPER -DEFI_DEBUG

%.efi: 
	$(CC.info)
	@$(CC) $(LDFLAGS) -o $@ $^

