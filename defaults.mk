# vim: set ft=make:ts=4:sw=4:nowrap

ARCH := x86_64
ABI := elf
TARGET := $(ARCH)-$(ABI)
CC := $(TARGET)-gcc
CXX := $(TARGET)-g++
AS := $(TARGET)-as
LD := $(TARGET)-ld
OBJCOPY := objcopy

CFLAGS += -ffreestanding -mno-red-zone -ggdb -std=c2x \
          -Wall -Wextra -Werror -fno-stack-protector \
	  -Wpedantic

CXXFLAGS += -ffreestanding -mno-red-zone -ggdb \
	    -Wall -Wextra -Werror -fno-stack-protector

LDFLAGS += -nostdlib
LIBDIRS += -L$(dir $(shell $(CC) -print-libgcc-file-name))
LOADLIBES += -lgcc

