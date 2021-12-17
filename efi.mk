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

CFLAGS += -fPIC -fshort-wchar -maccumulate-outgoing-args -mno-avx -mno-sse \
          -mno-mmx -funsigned-char -Wno-pointer-sign

OBJCOPY.efi = $(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
			-j .dynsm -j .rel -j .rela -j .reloc \
			--target=efi-app-$(GNUEFIARCH) $< $@

OBJCOPY.debug.efi = $(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic \
				  -j .dynsym -j .rel -j .rela -j .reloc -j .debug_info \
				  -j .debug_abbrev -j .debug_loc -j .debug_aranges \
				  -j .debug_line -j .debug_macinfo -j .debug_str \
				  --target=efi-app-$(GNUEFIARCH) $< $@

LINK._efi.so = $(LD) $(LDFLAGS) -znocombreloc -Bsymbolic -shared --no-undefined \
               $(LIBDIRS) -L$(EFILIBDIR) -T $(EFILDSCRIPT) \
               $(filter %.o, $^) -lgnuefi -lefi $(LOADLIBES) -o $@

%.efi: %_efi.so
	$(OBJCOPY.info)
	@$(OBJCOPY.efi)

%.debug.efi: %_efi.so
	$(OBJCOPY.info)
	@$(OBJCOPY.debug.efi)

%_efi.so: $(EFILDSCRIPT) $(EFICRT)
	$(LINK.info)
	@$(LINK._efi.so)

