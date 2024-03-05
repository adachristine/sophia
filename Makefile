BOOT_DIR := boot
BOOT_FILE := $(BOOT_DIR)/kjarna.efi

SERVICE_DIR := service
SERVICE_FILE := $(SERVICE_DIR)/kjarna.os

TARGETS := $(SERVICE_FILE) $(BOOT_FILE)
DIRS := $(SERVICE_DIR) $(BOOT_DIR)

DIRS_CLEAN := $(addprefix clean-,$(DIRS))
DIRS_DISTCLEAN := $(addprefix dist,$(DIRS_CLEAN))

all: $(TARGETS)
clean: $(DIRS_CLEAN)
distclean: $(DIRS_DISTCLEAN)

$(DIRS_CLEAN): clean-%:
	$(MAKE) -C $* clean

$(DIRS_DISTCLEAN): distclean-%:
	$(MAKE) -C $* distclean

$(TARGETS):
	$(MAKE) -C $(dir $@)

.PHONY: all clean distclean $(DIRS) $(DIRS_CLEAN) $(DIRS_DISTCLEAN)

