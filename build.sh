#!/usr/bin/bash
set -xe

CFLAGS="-Wall -Wextra"
OUTPUT="bin/qm"
MAIN="src/qm.c"

mkdir -p bin

gcc $MAIN -o $OUTPUT $CFLAGS
