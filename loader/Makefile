include ../defaults.mk
include ../efi.mk

VPATH := ../lib

IMAGE := loader.efi
BUILD := $(IMAGE) $(IMAGE:.efi=.debug.efi)

CPPFLAGS += -I../api

all: $(BUILD)

OBJS := main_efi.o elf64.o memcmp.o memmove.o
DEPS := $(OBJS:.o=.d)

IMAGE_SO := $(IMAGE:.efi=_efi.so)
$(IMAGE_SO): $(OBJS)
.INTERMEDIATE: $(IMAGE_SO)

CLEANLIST := $(wildcard $(BUILD) $(OBJS))
DCLEANLIST := $(CLEANLIST) $(wildcard $(DEPS))

include ../rules.mk
-include $(DEPS)

