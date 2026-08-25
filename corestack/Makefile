# =============================================================================
# // CORESTACK // Share library for other projects to build against.
#
# Expected Layout:
#	auth/		certificates and keys
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
# Usage:
#	make				runs 'all' by default, builds everything
#	make test			runs all tests
#	make unit			runs unit tests
#	make integration	runs integration tests
# 	make clean 			wipe build/ folder
#	make lint			automatically format code to style guide
# =============================================================================
# Blue heart emoji
INIT_MSG := \U0001F499
PROJECT_NAME := Corestack
COMMON_CFLAGS := -Wall -Wextra -g -pthread

TESTS_DIR    := tests
TEST_BIN_DIR := build/tests

include ../common.mk

# ----------------------------------------------------------------------
# Testing
# Placeholder for now, to change later!
# ----------------------------------------------------------------------
# Test-only include path: lets libbattleroyale tests #include "protocol.h" directly
TEST_CFLAGS := $(CFLAGS) -Isrc/lib/libbattleroyale

# Not using this as we wanna build everything including the linked files
# TARGET_STATIC := $(LIB_OUT)/libbattleroyale.a

# Auto-discover every .c file under tests/libbattleroyale/
TEST_SRCS := $(wildcard $(TESTS_DIR)/libbattleroyale/*.c)
TEST_BINS := $(patsubst $(TESTS_DIR)/%.c, $(TEST_BIN_DIR)/%, $(TEST_SRCS))

.PHONY: test

test: $(TEST_BINS)
	@echo "TEST_SRCS = $(TEST_SRCS)"
	@echo "TEST_BINS = $(TEST_BINS)"
	@echo "Running tests..."
	@for bin in $(TEST_BINS); do \
		echo "--- $$bin ---"; \
		./$$bin || exit 1; \
	done

$(TEST_BIN_DIR)/%: $(TESTS_DIR)/%.c | $(ALL_LIB_ARCHIVES) $(TEST_BIN_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)
	@echo "Built test binary: $@"

$(TEST_BIN_DIR):
	mkdir -p $@