include ../../defaults.mk
include ../defaults.mk

VPATH := ../../lib ../

TARGET := efi.os
.DEFAULT: $(TARGET)

# EFI API for temporary things

EFIAPIDIR := /usr/local/include/efi

CPPFLAGS += -I../../api -I$(EFIAPIDIR) -I$(EFIAPIDIR)/$(ARCH) -I$(EFIAPIDIR)/protocol
CFLAGS += -Wno-unused-const-variable -Wno-unused-function -fvisibility=hidden \
	  -g -fshort-wchar -mgeneral-regs-only

OBJS := entry_x86_64.o reloc_x86_64.o kc_main.o elf64.o memcmp.o memcpy.o \
	memmove.o memset.o
$(OBJS): CFLAGS += -fPIC

DEPS := $(patsubst %.o,%.d,$(OBJS))

$(TARGET): $(LDSCRIPT) $(MAKEFILE_LIST)
$(TARGET): $(OBJS)

CLEANLIST := $(wildcard $(OBJS) $(TARGET)) 
DCLEANLIST := $(wildcard $(DEPS))

include ../../rules.mk
-include $(DEPS)

.DEFAULT: $(TARGET)

