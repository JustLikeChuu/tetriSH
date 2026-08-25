# Shared build system for Corestack, Tetris & Bomberman
# Automatically discovers and builds all shared libraries and any binary found under src/
#
# Remember to hook BEFORE include ../common
#
# Expected Layout:
#	build/		compiled outputs (auto-created)
#		/bin/	compiled binaries
#		/lib/	static libraries
#		/obj/	intermediate .o files
#	include		headers
#		/lib/	library-specific headers
#	src			.c source files
#		/lib/	library source files
#	tests		unit tests
#
# Convention:
#   All directories under src/lib/ MUST start with 'lib' (eg: libssh/)
#   All directories not inside /lib but under src/ become binaries (eg: logd/)
#	Files without a directory also work, just follow the directory convention

# ----------------------------------------------------------------------
# Toolchain
# ----------------------------------------------------------------------
# GCC Compiler
CC := gcc

# Archiver that builds .o files into static library
# r = insert/replace, c=create if missing, s=add symbol index
AR := ar rcs

# ----------------------------------------------------------------------
# Directories
# ----------------------------------------------------------------------
SRC_DIR    ?= ./src
LIB_SUBDIR ?= lib
INC_DIR    ?= include
OBJ_DIR    ?= ./build/obj
LIB_OUT    ?= ./build/lib
BIN_OUT    ?= ./build/bin

TESTS_DIR  ?= ./tests
UNITY_DIR  ?= $(TESTS_DIR)/unity

# ----------------------------------------------------------------------
# Corestack linkage (empty for corestack itself)
# ----------------------------------------------------------------------
CS_INC        ?=
CS_LIB        ?=
CS_LDLIBS     ?=
COMMON_CFLAGS ?= -Wall -Wextra -g -pthread

SYS_LDLIBS := -lssh -lcrypto -lm

# ----------------------------------------------------------------------
# Compiler / Linker Flags
# ----------------------------------------------------------------------
# Compile Flags .c->.o (contains -I flags so headers can be found, plus common flags for compilation support)
CFLAGS  := -I$(INC_DIR) -I$(INC_DIR)/$(LIB_SUBDIR) $(if $(CS_INC),-I$(CS_INC)) $(COMMON_CFLAGS)
# Linker Flags multiple .o + libraries -> 1 binary (where to search for library files + put library names here!)
LDFLAGS := -L$(LIB_OUT) $(if $(CS_LIB),-L$(CS_LIB)) $(EXTRA_LDFLAGS)

# ----------------------------------------------------------------------
# Library Auto-Discovery (src/lib/<libname>)
# Scans src/lib for subdirectories/files, and builds them as static libraries
# ----------------------------------------------------------------------
## ** Directory-based libs (eg: src/lib/libtest/ -> build/lib/libtest.a) **
# Lists every subdirectory and strips trailing slash off each result
_LIB_SRCDIRS_ALL := $(patsubst %/,%,$(wildcard $(SRC_DIR)/$(LIB_SUBDIR)/*/))
# Loops over every directory found and drop those without .c files
_LIB_SRCDIRS     := $(foreach d,$(_LIB_SRCDIRS_ALL),$(if $(wildcard $(d)/*.c),$(d)))
# Get folder names by stripping leading path to each directory
LIB_NAMES        := $(notdir $(_LIB_SRCDIRS))

# ** Flat File libs (eg: src/lib/libtest.c -> build/lib/libtest.a) **
# Lists every .c file sitting directly in /src/lib/
FLAT_LIB_SRCS  := $(wildcard $(SRC_DIR)/$(LIB_SUBDIR)/*.c)
# Get file names by stripping leading path and .c extension from each file
FLAT_LIB_NAMES := $(patsubst $(SRC_DIR)/$(LIB_SUBDIR)/%.c,%,$(FLAT_LIB_SRCS))

# ** Linker Flags **
# Converts every library name into a -l flag, stripping 'lib' prefix (eg: libtest -> -ltest)
LOCAL_LDLIBS := $(patsubst lib%,-l%,$(LIB_NAMES) $(FLAT_LIB_NAMES))
# Assigns final set of -l flags to LDLIBS, including corestack libraries
LDLIBS       := $(LOCAL_LDLIBS) $(CS_LDLIBS) $(SYS_LDLIBS)

# Final container of all libraries, empty for now
ALL_LIB_ARCHIVES :=

# Directory Library Template: Subdirectory -> Archive
# $(1) is the argument supplied to this function, the library name (eg: libtest)
# Note that we include each directory's own path to the CFLAGS to
# (eg: Allow src/lib/libhtttp/client.c to #include "client.h" instead of "libhtttp/client.h")
# 1) Convert every .c file found in directory to an object file (eg: src/lib/libtest/test.c -> build/obj/libtest/test.o)
# 2) Compile corresponding .c file whenever a .o is needed, creating the directory if hasn't existed yet
# 3) Bundle all .o files into the static archive (eg: build/lib/libtest.a), where $$@ is the archive, $$^ is all the .o files
# 4) Append built archive to running list of libraries to build at top-level
# See: https://devdocs.io/gnu_make/automatic-variables (we write $$ to escape inside a define block)
define LIB_template =
$(1)_SRC := $$(wildcard $(SRC_DIR)/$(LIB_SUBDIR)/$(1)/*.c)
$(1)_OBJ := $$(patsubst $(SRC_DIR)/$(LIB_SUBDIR)/$(1)/%.c, \
             $(OBJ_DIR)/$(1)/%.o,$$($(1)_SRC))
$(1)_CFLAGS := $(CFLAGS) -I$(INC_DIR)/$(LIB_SUBDIR)/$(1)

$(OBJ_DIR)/$(1)/%.o: $(SRC_DIR)/$(LIB_SUBDIR)/$(1)/%.c
	@mkdir -p $$(dir $$@)
	$(CC) $$($(1)_CFLAGS) -c $$< -o $$@

$(LIB_OUT)/$(1).a: $$($(1)_OBJ)
	@mkdir -p $(LIB_OUT)
	$(AR) $$@ $$^

ALL_LIB_ARCHIVES += $(LIB_OUT)/$(1).a
endef

# Runs template once per directory library name
$(foreach lib,$(LIB_NAMES),$(eval $(call LIB_template,$(lib))))

# Flat Library Template: Single .c file -> Archive
# 1) Compile single flat source file directory into a .o
# 2) Archive the single .o into .a
# 3) Append built archive to running list of libraries to build
define FLAT_LIB_template =
$(OBJ_DIR)/$(1).o: $(SRC_DIR)/$(LIB_SUBDIR)/$(1).c
	@mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) -c $$< -o $$@

$(LIB_OUT)/$(1).a: $(OBJ_DIR)/$(1).o
	@mkdir -p $(LIB_OUT)
	$(AR) $$@ $$^

ALL_LIB_ARCHIVES += $(LIB_OUT)/$(1).a
endef

# Runs template once per flat library
$(foreach lib,$(FLAT_LIB_NAMES),$(eval $(call FLAT_LIB_template,$(lib))))

# ----------------------------------------------------------------------
# Binary Auto-Discovery (src/<name>)
# Builds everything under src/ that is NOT inside src/lib as binaries
# ----------------------------------------------------------------------
# ** Directory-based Binaries (eg: src/test/ -> build/bin/test) **
# Lists every subdirectory inside src/ and strips trailing slash off each result
_ALL_SRCDIRS_ALL := $(patsubst %/,%,$(wildcard $(SRC_DIR)/*/))
# Drop any directories inside /lib folder
_BIN_SRCDIRS_ALL := $(filter-out $(SRC_DIR)/$(LIB_SUBDIR),$(_ALL_SRCDIRS_ALL))
# Drop any directories without .c files
_BIN_SRCDIRS     := $(foreach d,$(_BIN_SRCDIRS_ALL),$(if $(wildcard $(d)/*.c),$(d)))
# Get file names by stripping leading paths
BIN_NAMES        := $(notdir $(_BIN_SRCDIRS))

# ** Flat File Binaries (eg: src/test.c -> build/bin/test) **
# Every .c file sitting directly in /src
FLAT_BIN_SRCS  := $(wildcard $(SRC_DIR)/*.c)
# Get file names by stripping leading path and .c extension from each file
FLAT_BIN_NAMES := $(patsubst $(SRC_DIR)/%.c,%,$(FLAT_BIN_SRCS))

# Final container of all binaries, empty for now
ALL_BINS :=

# Extra Dependency Flags (eg: RayLib, stb_image)
# Empty by default.
EXTRA_DEP_BINS    ?=
EXTRA_DEP_CFLAGS  ?=
EXTRA_DEP_LDFLAGS ?=
EXTRA_DEP_DEPS    ?=

# Directory Binary Template: Subdirectory -> Binary
# Similar to library templates,
# 1) Create list of every .c file found in directory
# 2) Declare binary as target with prerequisites src (normal), libraries and
#    any EXTRA_DEP_DEPS this binary opted into (order-only)
# ie: src is rebuilt whenever it's modified, libraries/deps are rebuilt only if its existence changes
# See: https://devdocs.io/gnu_make/prerequisite-types
# 3) Also include each directory's own path to the search path
# (eg: Allow src/tetrisu/test.c to #include "test.h" instead of "tetrisu/test.h")
# 4) Compile every file to binary, including linker paths and this binary's
#    extra flags (empty unless it's listed in EXTRA_DEP_BINS)
# 5) Append compiled binary to running list of binaries to build at top-level
define BIN_template =
$(1)_SRC := $$(wildcard $(SRC_DIR)/$(1)/*.c)
$(1)_CFLAGS        := $(CFLAGS) -I$(INC_DIR)/$(1)
$(1)_EXTRA_CFLAGS  := $$(if $$(filter $(EXTRA_DEP_BINS),$(1)),$(EXTRA_DEP_CFLAGS))
$(1)_EXTRA_LDFLAGS := $$(if $$(filter $(EXTRA_DEP_BINS),$(1)),$(EXTRA_DEP_LDFLAGS))
$(1)_EXTRA_DEPS    := $$(if $$(filter $(EXTRA_DEP_BINS),$(1)),$(EXTRA_DEP_DEPS))

$(BIN_OUT)/$(1): $$($(1)_SRC) | $(ALL_LIB_ARCHIVES) $$($(1)_EXTRA_DEPS)
	@mkdir -p $$(dir $$@)
	$(CC) $$($(1)_CFLAGS) $$($(1)_EXTRA_CFLAGS) $$($(1)_SRC) -o $$@ \
		$(LDFLAGS) $(LDLIBS) $$($(1)_EXTRA_LDFLAGS)

ALL_BINS += $(BIN_OUT)/$(1)
endef

# Runs template once per directory binary name
$(foreach bin,$(BIN_NAMES),$(eval $(call BIN_template,$(bin))))

# Flat Binary Template: Single .c File -> Binary
# Identical to BIN_template except source is a single named file ($$<) rather than
# a wildcard over a directory
define FLAT_BIN_template =
$(1)_EXTRA_CFLAGS  := $$(if $$(filter $(EXTRA_DEP_BINS),$(1)),$(EXTRA_DEP_CFLAGS))
$(1)_EXTRA_LDFLAGS := $$(if $$(filter $(EXTRA_DEP_BINS),$(1)),$(EXTRA_DEP_LDFLAGS))
$(1)_EXTRA_DEPS    := $$(if $$(filter $(EXTRA_DEP_BINS),$(1)),$(EXTRA_DEP_DEPS))

$(BIN_OUT)/$(1): $(SRC_DIR)/$(1).c | $(ALL_LIB_ARCHIVES) $$($(1)_EXTRA_DEPS)
	@mkdir -p $$(dir $$@)
	$(CC) $(CFLAGS) $$($(1)_EXTRA_CFLAGS) $$< -o $$@ \
		$(LDFLAGS) $(LDLIBS) $$($(1)_EXTRA_LDFLAGS)

ALL_BINS += $(BIN_OUT)/$(1)
endef

# Runs template once per flat binary
$(foreach bin,$(FLAT_BIN_NAMES),$(eval $(call FLAT_BIN_template,$(bin))))

# ----------------------------------------------------------------------
# Top-Level Targets
# ----------------------------------------------------------------------
.PHONY: all init clean lint kill-server
.DEFAULT_GOAL := all

# All project .c/.h under src/, include/, tests/
# Used for linting
SOURCES := $(shell find $(SRC_DIR) $(INC_DIR) $(TESTS_DIR) \
	-path $(UNITY_DIR) -prune -o \
	\( -name '*.c' -o -name '*.h' \) -print)

all: $(ALL_LIB_ARCHIVES) $(ALL_BINS) $(TARGET)

# Generate compile_commands.json for Intellisense.
# INIT_MSG & POST_INIT is set per-project. 
# 	INIT_MSG: Banner printed by make init. Usually an escaped emoji!
# 	POST_INIT: Commands to run after init, like logging
init:
	make clean
	bear -- make
	@printf "\u005B$(INIT_MSG) $(PROJECT_NAME)\u005D Created runtime directories and compile_commands for Intellisense!\n"

# ----------------------------------------------------------------------
# Cleaning
# ----------------------------------------------------------------------
clean:
	rm -rf ./build $(UNIT_BIN)
	rm -f $(TARGET)

# ----------------------------------------------------------------------
# Fix variable naming via clang-tidy,
# => we run run-clang-tidy instead of --fix so we apply fixes in one pass to avoid conflicts
# then running clang-format to fix layout (spacing & brackets)
# tidy -> format as renames can change line lengths
# ----------------------------------------------------------------------
lint: init # Generate compile_commands.json first
	run-clang-tidy -p . -fix $(SOURCES)
	clang-format -i $(SOURCES)


# --------------------------
# Server Launching
# --------------------------
SERVER_BIN ?=
CLIENT_BIN ?=
SERVER_PORT := 6700

kill-server:
	@pkill -9 -x "${SERVER_BIN}" 2>/dev/null; \
	fuser -k ${SERVER_PORT}/tcp 2>/dev/null; \
	echo "Killed any ${SERVER_BIN} process and freed port ${SERVER_PORT}."