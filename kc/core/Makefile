include ../../defaults.mk
include ../defaults.mk

VPATH := ../../lib ../ cpu/ memory/

TARGET := kernel.os
.DEFAULT: $(TARGET)

CPPFLAGS += -I../../api -I../api -I../../lib -I.
CFLAGS += -Wno-unused-const-variable -Wno-unused-function -fvisibility=hidden -g

SOBJS := 
GOBJS := entry_x86_64.o reloc_x86_64.o dynamic_x86_64.o \
	 kprintf.o serial.o port.o mmu.o kc_main.o memory.o memset.o memcpy.o \
	 memmove.o memcmp.o cpu.o vm_tree.o exceptions.o panic.o msr.o task.o \
	 cpu_task.o irq.o kcc_memory.o string.o pic8259.o pic8259_isr.o \
	 pit8253.o page_early.o page_stack.o kstdio.o video.o i8042.o
IOBJS :=

# __attribute__((interrupt)) requires -mgeneral-regs-only
$(GOBJS): CFLAGS += -mgeneral-regs-only
$(GOBJS): CFLAGS += -mcmodel=small

OBJS := $(SOBJS) $(GOBJS) $(IOBJS)
DEPS := $(patsubst %.o,%.d,$(OBJS))

$(TARGET): $(LDSCRIPT) $(MAKEFILE_LIST)
$(TARGET): $(OBJS)

CLEANLIST := $(wildcard $(OBJS) $(TARGET)) 
DCLEANLIST := $(wildcard $(DEPS))

include ../../rules.mk
-include $(DEPS)

.DEFAULT: $(TARGET)

