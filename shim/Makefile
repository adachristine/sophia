include ../defaults.mk

VPATH := ../lib

TARGET := efi.os
.DEFAULT: $(TARGET)

# EFI API for temporary things

EFIAPIDIR := /usr/local/include/efi

CPPFLAGS += -I../api -I$(EFIAPIDIR) -I$(EFIAPIDIR)/$(ARCH) -I$(EFIAPIDIR)/protocol
CFLAGS += -Wno-unused-const-variable -Wno-unused-function -fvisibility=hidden \
	  -g -fshort-wchar -mgeneral-regs-only

LIBDIRS += -L$(dir $(shell $(CC) -print-libgcc-file-name))
LDSCRIPT := shim.ld
LDFLAGS += -z max-page-size=4096 --export-dynamic
LOADLIBES := -lgcc

OBJS := entry.o memcmp.o memcpy.o memmove.o memset.o
$(OBJS): CFLAGS += -fPIC -fPIE

DEPS := $(patsubst %.o,%.d,$(OBJS))

$(TARGET): $(LDSCRIPT) $(MAKEFILE_LIST)
$(TARGET): $(OBJS)

CLEANLIST := $(wildcard $(OBJS) $(TARGET)) 
DCLEANLIST := $(wildcard $(DEPS))

include ../rules.mk
-include $(DEPS)

.DEFAULT: $(TARGET)

