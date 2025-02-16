CC := gcc
CFLAGS := -Wall -Werror -Wextra -Wconversion -Wpedantic -Wstrict-prototypes -std=gnu17 -g -DDEBUG -O0

SRC_DIR := ./src
BUILD_DIR := ./bin
EXE := ${BUILD_DIR}/procman

SRC = $(SRC_DIR)/main.c $(SRC_DIR)/argparse.c $(SRC_DIR)/procman.c
	

all: $(EXE)

# Build executable
$(EXE): $(SRC)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $^ -o $@

prog: $(SRC_DIR)/prog.c
	mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $(BUILD_DIR)/$@

.PHONY: clean
clean:
	rm -f $(EXE)
	rm -f $(BUILD_DIR)/prog

# Test with prog
.PHONY:
.SILENT:
test: all prog
	cd $(BUILD_DIR)
	exec ./$(basename $(EXE))
