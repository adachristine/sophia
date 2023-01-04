# vim: ft=make:nowrap:ts=4:sw=4

ifeq ($(ARCH),x86_64)
 GNUEFIARCH := x86_64
endif
ifeq ($(ARCH),i686)
 GNUEFIARCH := ia32
endif

%.efi: 
	$(CC.info)
	@$(CC) $(LDFLAGS) -o $@ $^

