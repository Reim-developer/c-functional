# c-functional — build the tests and examples.
#
#   make test    build + run everything with $(CC) at -std=c99
#   make check   full matrix: gcc and clang, each at -std=c99 and -std=c11
#   make clean   remove build artifacts

CC ?= cc
STD ?= c99
CFLAGS = -std=$(STD) -Wall -Wextra -Werror -pedantic -Iinclude

BUILD_DIR = build

TEST_BIN = $(BUILD_DIR)/test_cf-$(CC)-$(STD)
MULTI_BIN = $(BUILD_DIR)/test_multi-$(CC)-$(STD)
UNUSED_BIN = $(BUILD_DIR)/test_unused-$(CC)-$(STD)
EXAMPLE_BIN = $(BUILD_DIR)/quickstart-$(CC)-$(STD)

.PHONY: all test check clean

all: test

test: $(TEST_BIN) $(MULTI_BIN) $(UNUSED_BIN) $(EXAMPLE_BIN)
	./$(TEST_BIN)
	./$(MULTI_BIN)
	./$(UNUSED_BIN)
	./$(EXAMPLE_BIN)

$(TEST_BIN): tests/test_cf.c include/cf.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_cf.c -o $@

$(MULTI_BIN): tests/test_multi_a.c tests/test_multi_b.c tests/shared_int.h include/cf.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_multi_a.c tests/test_multi_b.c -o $@

$(UNUSED_BIN): tests/test_unused.c include/cf.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_unused.c -o $@

$(EXAMPLE_BIN): examples/quickstart.c include/cf.h
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) examples/quickstart.c -o $@

check:
	@for cc in gcc clang; do \
	  if command -v $$cc >/dev/null 2>&1; then \
	    for std in c99 c11; do \
	      echo "=== $$cc -std=$$std ==="; \
	      $(MAKE) --no-print-directory test CC=$$cc STD=$$std || exit 1; \
	    done; \
	  else \
	    echo "=== $$cc not found, skipping ==="; \
	  fi; \
	done

clean:
	rm -rf $(BUILD_DIR)
