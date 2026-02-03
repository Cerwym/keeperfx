#******************************************************************************
#  Free implementation of Bullfrog's Dungeon Keeper strategy game.
#******************************************************************************
#   @file installer.mk
#      A script used by GNU Make to create installers.
#  @par Purpose:
#      Defines make rules for creating installers (InnoSetup).
#  @par Comment:
#      InnoSetup can be run via Wine on Linux for CI/CD pipelines.
#  @author   KeeperFX Team
#  @date     03 Feb 2026
#  @par  Copying and copyrights:
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; either version 2 of the License, or
#      (at your option) any later version.
#
#******************************************************************************

# InnoSetup compiler (can be overridden for Wine on Linux)
ISCC ?= iscc
WINE ?= wine

# Installer output settings
INSTALLER_DIR = installer
INSTALLER_NAME = $(INSTALLER_DIR)/keeperfx-$(subst $(space),_,$(subst .,_,$(VER_STRING)))-setup.exe

# Check if we're on Linux and need to use Wine
ifneq (,$(findstring Linux,$(shell uname -s)))
  ISCC_CMD = $(WINE) "$(INNOSETUP_PATH)/ISCC.exe"
else
  ISCC_CMD = $(ISCC)
endif

.PHONY: installer gog-installer clean-installer

$(INSTALLER_DIR):
	$(MKDIR) $@

# Build the installer
# Requires that 'package' target has been run first to populate pkg/ directory
installer: $(PKG_FILES) | $(INSTALLER_DIR)
	@$(ECHO) "Building installer with InnoSetup..."
	@$(ECHO) "Version: $(VER_STRING)"
	@export KEEPERFX_VERSION=$(VER_STRING) && \
	if command -v iscc >/dev/null 2>&1; then \
		iscc keeperfx-installer.iss; \
	elif command -v wine >/dev/null 2>&1 && [ -f "$(HOME)/.wine/drive_c/Program Files (x86)/Inno Setup 6/ISCC.exe" ]; then \
		wine "$(HOME)/.wine/drive_c/Program Files (x86)/Inno Setup 6/ISCC.exe" keeperfx-installer.iss; \
	else \
		echo "Error: InnoSetup not found. Install InnoSetup or set up Wine with InnoSetup."; \
		echo "On Linux/CI: Use the setup-innosetup action or install via Wine"; \
		exit 1; \
	fi
	@$(ECHO) "Installer created: $(INSTALLER_NAME)"

# Alias for GOG-specific installer (currently same as regular installer)
gog-installer: installer

# Clean installer artifacts
clean-installer:
	$(RM) -r $(INSTALLER_DIR)

#******************************************************************************
