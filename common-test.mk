# Shared cmocka-based test scaffold for tetris and bomberman.
# Include AFTER ../common.mk (needs CFLAGS, LDFLAGS, LDLIBS, LIB_OUT, etc. from it)
# Set PROJECT_NAME before including this file so we can label test output

UNIT_DIR := $(TESTS_DIR)/unit
INT_DIR  := $(TESTS_DIR)/integration
UNIT_BIN := $(UNIT_DIR)/bin

UNIT_SRCS := $(wildcard $(UNIT_DIR)/test_*.c)
UNIT_BINS := $(UNIT_SRCS:$(UNIT_DIR)/%.c=$(UNIT_BIN)/%)

# Tests never link raylib — test game logic through the libs directly.
TEST_CFLAGS  := $(CFLAGS)
TEST_LDFLAGS := -L$(LIB_OUT) $(if $(CS_LIB),-L$(CS_LIB))
TEST_LDLIBS  := $(LDLIBS) -lcmocka

.PHONY: unit integration test

$(UNIT_BIN)/test_%: $(UNIT_DIR)/test_%.c | $(ALL_LIB_ARCHIVES)
	@mkdir -p $(UNIT_BIN)
	$(CC) $(TEST_CFLAGS) $< -o $@ $(TEST_LDFLAGS) $(TEST_LDLIBS)

unit: $(UNIT_BINS)
	@echo "==> Running $(PROJECT_NAME) unit tests"
	@pass=0; fail=0; \
	for t in $(UNIT_BINS); do \
	  echo "--- $$t ---"; \
	  if $$t; then pass=$$((pass+1)); else fail=$$((fail+1)); fi; \
	done; \
	echo ""; \
	echo "Unit tests: $$pass passed, $$fail failed"; \
	test $$fail -eq 0

integration: all
	@echo "==> Running $(PROJECT_NAME) integration tests"
	@pass=0; fail=0; \
	for s in $(INT_DIR)/*.sh; do \
	  [ -f "$$s" ] || continue; \
	  echo "--- $$s ---"; \
	  if bash $$s; then pass=$$((pass+1)); else fail=$$((fail+1)); fi; \
	done; \
	echo ""; \
	echo "Integration tests: $$pass passed, $$fail failed"; \
	test $$fail -eq 0

test: unit integration