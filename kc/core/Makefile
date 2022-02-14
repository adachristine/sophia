include ../../defaults.mk
include ../defaults.mk

VPATH := ../../lib ../ cpu/

TARGET := kernel.os
.DEFAULT: $(TARGET)

CPPFLAGS += -I../../api -I.
CFLAGS += -Wno-unused-const-variable -Wno-unused-function -fvisibility=hidden -g

SOBJS := 
GOBJS := entry_x86_64.o reloc_x86_64.o kprint.o serial.o port.o \
	 kc_main.o memory.o memset.o memcpy.o memmove.o memcmp.o cpu.o \
	 vm_tree.o exceptions.o panic.o msr.o task.o cpu_task.o \
	 kcc_memory.o string.o pic8259.o pic8259_isr.o pit8253.o timer.o
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

