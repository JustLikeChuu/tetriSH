# Shared dependency fetching for third-party libraries.
# Opt-in, just include this file AFTER ../common.mk

# Source Tarball that needs to be built (eg: RayLib)
# Set, then call template with your chosen name:
# 	[NAME]_URL: URL to tarball
# 	[NAME]_ARTIFACT: Where the tarball should extract to
# 	[NAME]_BUILD_CMD: Command to run to build the tarball
#	$(eval $(call FETCH_TARBALL_template,[NAME]))
define FETCH_TARBALL_template =
$($(1)_ARTIFACT):
	@echo "==> Downloading $(1) ($($(1)_URL))..."
	@mkdir -p $(DEPS_DIR)
	@if command -v curl > /dev/null 2>&1; then \
	    curl -fsSL $($(1)_URL) | tar -xz -C $(DEPS_DIR); \
	elif command -v wget > /dev/null 2>&1; then \
	    wget -qO- $($(1)_URL) | tar -xz -C $(DEPS_DIR); \
	else \
	    echo "Error: neither curl nor wget found."; \
	    echo "Install one, or install $(1) manually and re-run make."; \
	    exit 1; \
	fi
	@echo "==> Building $(1) (this may take a moment)..."
	$($(1)_BUILD_CMD)
	@echo "==> $(1) ready at $($(1)_ARTIFACT)"
endef

# A single vendored file without built steps (eg: a simple .h)
# Set the template:
# 	[NAME]_URL: URL to file
# 	[NAME]_ARTIFACT: Where the file should extract to
#	$(eval $(call FETCH_FILE_template,NAME))
define FETCH_FILE_template =
$($(1)_ARTIFACT):
	@echo "==> Downloading $(1) ($($(1)_URL))..."
	@mkdir -p $$(dir $($(1)_ARTIFACT))
	@if command -v curl > /dev/null 2>&1; then \
	    curl -fsSL $($(1)_URL) -o $($(1)_ARTIFACT); \
	elif command -v wget > /dev/null 2>&1; then \
	    wget -qO $($(1)_ARTIFACT) $($(1)_URL); \
	else \
	    echo "Error: neither curl nor wget found."; \
	    echo "Install one, or place a copy of $(1) at $($(1)_ARTIFACT) yourself."; \
	    exit 1; \
	fi
endef
