export PYTHONDONTWRITEBYTECODE := 1
CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
CPPFLAGS ?= -Icanonical_c11/include
BUILD := build
BIN := $(BUILD)/dacb
TESTBIN := $(BUILD)/test_c

.PHONY: all clean test report twin cross exhaustive verify

all: $(BIN)

$(BUILD):
	mkdir -p $(BUILD)

$(BIN): canonical_c11/src/dacb.c canonical_c11/src/main.c canonical_c11/include/dacb.h | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) canonical_c11/src/dacb.c canonical_c11/src/main.c -o $@

$(TESTBIN): canonical_c11/src/dacb.c tests/test_c.c canonical_c11/include/dacb.h | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) canonical_c11/src/dacb.c tests/test_c.c -o $@

test: $(TESTBIN) $(BIN)
	$(TESTBIN)
	python3 tests/test_python_twin.py
	python3 tests/test_cli_contract.py

report: $(BIN)
	$(BIN) report

twin:
	python3 python_twin/dacb_twin.py report

cross: $(BIN)
	mkdir -p $(BUILD)/reports
	$(BIN) report > $(BUILD)/reports/c.report
	python3 python_twin/dacb_twin.py report > $(BUILD)/reports/python.report
	cmp $(BUILD)/reports/c.report $(BUILD)/reports/python.report

exhaustive: $(BIN)
	$(BIN) exhaustive 2 10

verify:
	bash scripts/verify_release.sh

clean:
	rm -rf $(BUILD)
