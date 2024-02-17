include ../defaults.mk
include ../efi.mk

VPATH := ../lib efi/

IMAGE := kjarna.efi

CPPFLAGS += -I../api -I../lib

CFLAGS += --target=$(GNUEFIARCH)-unknown-windows -fshort-wchar -mno-avx \
	  -mno-sse -mno-mmx -funsigned-char -Wno-pointer-sign -ggdb \
	  -fno-ms-extensions

LDFLAGS := --target=$(GNUEFIARCH)-unknown-windows -nostdlib -ggdb \
	   -Wl,-entry:_start,-subsystem:efi_application -fuse-ld=lld-link

all: $(IMAGE)

CRT_OBJS := start.o
LIB_OBJS := memcmp.o memset.o memcpy.o memmove.o string.o elf64.o
EFI_OBJS := file.o memory.o protocol.o image.o
APP_OBJS := main.o

OBJS := $(CRT_OBJS) $(LIB_OBJS) $(EFI_OBJS) $(APP_OBJS)
DEPS := $(OBJS:.o=.d)

$(IMAGE): $(OBJS)

CLEANLIST := $(wildcard $(BUILD) $(OBJS) $(IMAGE) $(IMAGE:.efi=.pdb))
DCLEANLIST := $(CLEANLIST) $(wildcard $(DEPS))

include ../rules.mk
-include $(DEPS)

