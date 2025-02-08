CC := gcc
CFLAGS := -Wall -Werror -Wextra -Wconversion -Wpedantic -Wstrict-prototypes -std=gnu17

SRC_DIR := ./src
BUILD_DIR := ./build
EXE := ${BUILD_DIR}/procman

SRC = $(shell find $(SRC_DIR) -name "*.c" -type f)
	

all: $(EXE)

# Build executable
$(EXE): $(SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(EXE)
