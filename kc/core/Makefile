include ../../defaults.mk
include ../defaults.mk

VPATH := ../../lib ../ cpu/ memory/

TARGET := kernel.os
.DEFAULT: $(TARGET)

CPPFLAGS += -I../../api -I../api -I../../lib -I.
CFLAGS += -mcmodel=small -Wno-unused-const-variable -Wno-unused-function -fvisibility=hidden -g

AOBJS := entry_x86_64.o reloc_x86_64.o dynamic_x86_64.o
COBJS := mmu.o cpu_task.o irq.o exceptions.o cpu.o msr.o mmu.o port.o
POBJS := pic8259.o pic8259_isr.o pit8253.o i8042.o
GOBJS := serial.o kc_main.o memory.o vm_tree.o panic.o task.o \
	     kcc_memory.o page_early.o page_stack.o video.o
LOBJS := kprintf.o memset.o memcpy.o memmove.o memcmp.o kstdio.o string.o

OBJS := $(AOBJS) $(COBJS) $(POBJS) $(GOBJS) $(LOBJS)
DEPS := $(patsubst %.o,%.d,$(OBJS))


$(TARGET): $(LDSCRIPT) $(MAKEFILE_LIST)
$(TARGET): $(OBJS)

CLEANLIST := $(wildcard $(OBJS) $(TARGET)) 
DCLEANLIST := $(wildcard $(DEPS))

include ../../rules.mk
-include $(DEPS)

.DEFAULT: $(TARGET)

