#!/bin/sh

CC=gcc
CFLAGS='-Wall -Werror -Wextra -Wconversion -Wpedantic -Wstrict-prototypes -std=gnu17 -g'
SRC_DIR=./src
BUILD_DIR=./bin
EXE=$BUILD_DIR/shell

mkdir -p $BUILD_DIR

# Compile main executable test binary
SRC="$SRC_DIR/main.c $SRC_DIR/argparse.c $SRC_DIR/procman.c"
$CC $CFLAGS $SRC -o $EXE

# Compile prog test binary
$CC $CFLAGS $SRC_DIR/prog.c -o $BUILD_DIR/prog
