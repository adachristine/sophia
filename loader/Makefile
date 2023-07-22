include ../defaults.mk
include ../efi.mk

VPATH := ../lib

IMAGE := loader.efi

CPPFLAGS += -I../api -I../lib

CFLAGS += --target=$(GNUEFIARCH)-unknown-windows -fshort-wchar -mno-avx \
	  -mno-sse -mno-mmx -funsigned-char -Wno-pointer-sign -ggdb \
	  -fno-ms-extensions

LDFLAGS := --target=$(GNUEFIARCH)-unknown-windows -nostdlib -ggdb \
	   -Wl,-entry:efi_main,-subsystem:efi_application -fuse-ld=lld-link

all: $(IMAGE)

OBJS := main_efi.o elf64.o memcmp.o memmove.o kprintf.o string.o memset.o \
		memcpy.o
DEPS := $(OBJS:.o=.d)

$(IMAGE): $(OBJS)

CLEANLIST := $(wildcard $(BUILD) $(OBJS) $(IMAGE) $(IMAGE:.efi=.pdb))
DCLEANLIST := $(CLEANLIST) $(wildcard $(DEPS))

include ../rules.mk
-include $(DEPS)

