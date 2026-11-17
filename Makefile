# c-functional — build the tests and examples.
#
#   make test          build + run everything with $(CC) at -std=c99
#   make check         full matrix: gcc and clang, each at -std=c99 and -std=c11
#                      (a missing compiler is skipped — convenient locally)
#   make check-strict  like check, but fails if gcc or clang is missing —
#                      for CI, where a silently skipped compiler would hide
#                      a broken build
#   make asan          tests + example under ASan + UBSan (gcc, C99)
#   make clean         remove build artifacts

CC ?= cc
STD ?= c99
CFLAGS = -std=$(STD) -Wall -Wextra -Werror -pedantic -Iinclude

BUILD_DIR = build

TEST_BIN = $(BUILD_DIR)/test_cf-$(CC)-$(STD)
MULTI_BIN = $(BUILD_DIR)/test_multi-$(CC)-$(STD)
UNUSED_BIN = $(BUILD_DIR)/test_unused-$(CC)-$(STD)
EXAMPLE_BIN = $(BUILD_DIR)/quickstart-$(CC)-$(STD)

.PHONY: all test check check-strict asan clean

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

check-strict:
	@for cc in gcc clang; do \
	  if ! command -v $$cc >/dev/null 2>&1; then \
	    echo "error: $$cc not found, refusing to skip in strict mode" >&2; \
	    exit 1; \
	  fi; \
	done
	$(MAKE) --no-print-directory check

asan:
	@mkdir -p $(BUILD_DIR)
	gcc -std=c99 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -Iinclude tests/test_cf.c -o $(BUILD_DIR)/test_cf-asan
	./$(BUILD_DIR)/test_cf-asan
	gcc -std=c99 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined -fno-sanitize-recover=all -Iinclude examples/quickstart.c -o $(BUILD_DIR)/quickstart-asan
	./$(BUILD_DIR)/quickstart-asan

clean:
	rm -rf $(BUILD_DIR)
