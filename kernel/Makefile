include ../defaults.mk

VPATH := ../lib

TARGET := kernel.os
.DEFAULT: $(TARGET)

CFLAGS += -Wno-unused-const-variable -Wno-unused-function -fvisibility=hidden -g

LIBDIRS += -L$(dir $(shell $(CC) -print-libgcc-file-name))
LDSCRIPT := kernel.ld
LDFLAGS += -z max-page-size=4096 --export-dynamic
LOADLIBES := -lgcc

SOBJS := 
GOBJS := entry.o kprint.o serial.o port.o main.o memory.o \
	 memset.o memcpy.o memmove.o memcmp.o cpu.o exceptions.o \
	 panic.o
IOBJS :=

$(SOBJS): CFLAGS += -fPIC
# __attribute__((interrupt)) requires -mgeneral-regs-only
$(IOBJS): CFLAGS += -mgeneral-regs-only
$(IOBJS) $(GOBJS): CFLAGS += -mcmodel=kernel

OBJS := $(SOBJS) $(GOBJS) $(IOBJS)
DEPS := $(patsubst %.o,%.d,$(OBJS))

$(TARGET): $(LDSCRIPT) $(MAKEFILE_LIST)
$(TARGET): $(OBJS)

CLEANLIST := $(wildcard $(OBJS) $(TARGET)) 
DCLEANLIST := $(wildcard $(DEPS))

include ../rules.mk
-include $(DEPS)

.DEFAULT: $(TARGET)

