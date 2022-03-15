
LIBDIRS += -L$(dir $(shell $(CC) -print-libgcc-file-name))
LDSCRIPT := ../kc.ld
LDFLAGS += -z max-page-size=4096 --export-dynamic -pic \
	   -z separate-code -pie -Bsymbolic -shared
LOADLIBES := -lgcc

