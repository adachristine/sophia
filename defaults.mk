# vim: set ft=make:ts=4:sw=4:nowrap

ARCH := x86_64
ABI := elf
TARGET := $(ARCH)-$(ABI)
CC := clang
CXX := clang++
AS := clang
LD := ld.bfd
OBJCOPY := objcopy

CFLAGS += -ffreestanding -mno-red-zone -ggdb -std=c2x \
          -Wall -Wextra -Werror -fno-stack-protector \
	  	  -Wpedantic -std=c2x

CXXFLAGS += -ffreestanding -mno-red-zone -ggdb \
	    -Wall -Wextra -Werror -fno-stack-protector

LDFLAGS += -nostdlib

