#!/usr/bin/env bash

# ============================================================
# NwC host compiler build script
#
# Expected layout:
#   compiler/
#     compilerMain.c
#     lexer/lexer.c
#     lexer/lexer.h
#     parser/parser.c
#     parser/parser.h
#     codegen/codegen.c
#     codegen/codegen.h
#     nwc_kernel.c
#     nwc_kernel.h
#     nwc_runtime.c
#     nwc_runtime.h
#     builded/
#       nwc                <- generated here
#
# NOTE:
#   nwc_kernel.* and nwc_runtime.* are kernel-side sources.
#   They are NOT linked into the NwC executable.
# ============================================================

cd "$(dirname "$0")" || exit 1

ROOT="$PWD"
OUT="$ROOT/builded"
NWC_OUT="$OUT/nwc"

mkdir -p "$OUT"

if ! command -v clang >/dev/null 2>&1; then
    echo "[NwC] ERROR: clang was not found in PATH."
    echo "[NwC] Add LLVM/Clang to PATH and run this again."
    exit 1
fi

if ! command -v lld-link >/dev/null 2>&1; then
    if ! command -v ld.lld >/dev/null 2>&1; then
        echo "[NwC] ERROR: LLVM LLD was not found."
        echo "[NwC] Expected lld-link or ld.lld in PATH."
        exit 1
    fi
fi

echo
echo "========================================"
echo "          Building NwC compiler"
echo "========================================"
echo

echo "[1/2] Compiling and linking..."

clang -std=c11 -O2 -Wall -Wextra -I"$ROOT" \
    "$ROOT/compilerMain.c" \
    "$ROOT/lexer/lexer.c" \
    "$ROOT/parser/parser.c" \
    "$ROOT/codegen/codegen.c" \
    -fuse-ld=lld \
    -o "$NWC_OUT"

if [ $? -ne 0 ]; then
    echo
    echo "[NwC] BUILD FAILED."
    exit 1
fi

echo
echo "[2/2] Checking output..."

if [ ! -f "$NWC_OUT" ]; then
    echo "[NwC] ERROR: nwc was not created."
    exit 1
fi

NWC_SIZE=$(stat -c%s "$NWC_OUT" 2>/dev/null || wc -c < "$NWC_OUT")

echo
echo "========================================"
echo "NwC build successful."
echo "Output: $NWC_OUT"
echo "Size  : $NWC_SIZE bytes"
echo "========================================"
echo

exit 0
