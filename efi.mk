# vim: ft=make:nowrap:ts=4:sw=4

ifeq ($(ARCH),x86_64)
 GNUEFIARCH := x86_64
endif
ifeq ($(ARCH),i686)
 GNUEFIARCH := ia32
endif

ifndef EFIAPIDIR
EFIAPIDIR := /usr/include/efi
endif

ifndef EFILIBDIR
EFILIBDIR := /usr/local/$(TARGET)/lib
endif

CPPFLAGS += -I$(EFIAPIDIR) -I$(EFIAPIDIR)/protocol -I$(EFIAPIDIR)/$(GNUEFIARCH) \
			-DEFI_CALL_WRAPPER -DGNU_EFI_USE_MS_ABI
%.efi: 
	$(CC.info)
	@$(CC) $(LDFLAGS) -o $@ $^

