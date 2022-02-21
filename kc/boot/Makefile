include ../../defaults.mk
include ../../efi.mk
include ../defaults.mk

VPATH := ../../lib ../

TARGET := efi.os
.DEFAULT: $(TARGET)

CPPFLAGS += -I../../api -I ../api
OBJS := entry_x86_64.o reloc_x86_64.o kc_main.o elf64.o memcmp.o memcpy.o \
	memmove.o memset.o loader_paging.o

DEPS := $(patsubst %.o,%.d,$(OBJS))

$(TARGET): $(LDSCRIPT) $(MAKEFILE_LIST)
$(TARGET): $(OBJS)

CLEANLIST := $(wildcard $(OBJS) $(TARGET)) 
DCLEANLIST := $(wildcard $(DEPS))

include ../../rules.mk
-include $(DEPS)

.DEFAULT: $(TARGET)

