include ../defaults.mk

LDFLAGS += -z max-page-size=4096 --export-dynamic -pic \
	   -z separate-code -pie -Bsymbolic -shared \
	   -z noexecstack -g

VPATH := ../lib

TARGET := kjarna.os
.DEFAULT: $(TARGET)

CPPFLAGS += -I../api/posix -I../api/libc -I../api -I. 

CFLAGS += -fPIC -fvisibility=hidden -mgeneral-regs-only

LDSCRIPT := kc.ld

CRT_OBJS := start_x86_64.o dynamic_x86_64.o elf64.o
LIB_OBJS := kjarna.o memcmp.o memcpy.o memmove.o memset.o string.o \
			stdio.o
APP_OBJS := main.o

OBJS := $(CRT_OBJS) $(LIB_OBJS) $(APP_OBJS) 

DEPS := $(patsubst %.o,%.d,$(OBJS))

$(TARGET): $(LDSCRIPT) $(MAKEFILE_LIST)
$(TARGET): $(OBJS)

CLEANLIST := $(wildcard $(OBJS) $(TARGET)) 
DCLEANLIST := $(wildcard $(DEPS))

include ../rules.mk
-include $(DEPS)

.DEFAULT: $(TARGET)

