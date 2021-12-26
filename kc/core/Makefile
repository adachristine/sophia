include ../../defaults.mk
include ../defaults.mk

VPATH := ../../lib ../

TARGET := kernel.os
.DEFAULT: $(TARGET)

CPPFLAGS += -I../../api
CFLAGS += -Wno-unused-const-variable -Wno-unused-function -fvisibility=hidden -g

SOBJS := 
GOBJS := entry_x86_64.o reloc_x86_64.o kprint.o serial.o port.o \
	 kc_main.o main.o memory.o memset.o memcpy.o memmove.o memcmp.o cpu.o \
	 exceptions.o panic.o
IOBJS :=

# __attribute__((interrupt)) requires -mgeneral-regs-only
$(IOBJS): CFLAGS += -mgeneral-regs-only
$(IOBJS) $(GOBJS): CFLAGS += -mcmodel=small -fPIC

OBJS := $(SOBJS) $(GOBJS) $(IOBJS)
DEPS := $(patsubst %.o,%.d,$(OBJS))

$(TARGET): $(LDSCRIPT) $(MAKEFILE_LIST)
$(TARGET): $(OBJS)

CLEANLIST := $(wildcard $(OBJS) $(TARGET)) 
DCLEANLIST := $(wildcard $(DEPS))

include ../../rules.mk
-include $(DEPS)

.DEFAULT: $(TARGET)
