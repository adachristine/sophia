LIBDIRS += -L$(dir $(shell $(CC) -print-libgcc-file-name))
LDSCRIPT := ../component.ld
LDFLAGS += -z max-page-size=4096 --export-dynamic -pic --no-dynamic-linker \
	   -z separate-code
LOADLIBES := -lgcc
