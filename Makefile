.PHONY: all clean $(DIRS_CLEAN)

BOOT_DIR := boot
BOOT_FILE := $(BOOT_DIR)/kjarna.efi

SERVICE_DIR := service
SERVICE_FILE := $(SERVICE_DIR)/kjarna.os

TARGETS := $(SERVICE_FILE) $(BOOT_FILE)
DIRS := $(SERVICE_DIR) $(BOOT_DIR)

DIRS_CLEAN := $(addprefix clean-,$(DIRS))
DIRS_DISTCLEAN := $(addprefix distclean-,$(DIRS))

all: $(TARGETS)

clean: $(DIRS_CLEAN)
distclean: $(DIRS_DISTCLEAN)

$(DIRS_CLEAN): clean-%:
	$(MAKE) -C $* clean

$(DIRS_DISTCLEAN): distclean-%:
	$(MAKE) -C $* distclean

$(TARGETS):
	$(MAKE) -C $(dir $@)


