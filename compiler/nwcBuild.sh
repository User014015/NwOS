#!/usr/bin/env bash

# ============================================================
# NwC compiler build script (step by step variant)
#
# Expected layout (run from the repository root):
#   compiler/
#     compilerMain.c
#     lexer/lexer.c
#     parser/parser.c
#     codegen/codegen.c
#     builded/            <- object files land here
#     nwc                 <- generated here
# ============================================================

cd "$(dirname "$0")/.." || exit 1

mkdir -p compiler/builded

error_exit() {
    echo
    echo "================================"
    echo "       BUILD FAILED"
    echo "================================"
    echo

    read -rp "Press Enter to continue..." _
    exit 1
}

echo "================================"
echo "       NwC Compiler Build"
echo "================================"
echo

echo "[1/3] Compiling lexer..."
clang compiler/lexer/lexer.c -c -o compiler/builded/lexer.o

if [ $? -ne 0 ]; then error_exit; fi

echo "[2/3] Compiling parser..."
clang compiler/parser/parser.c -c -o compiler/builded/parser.o

if [ $? -ne 0 ]; then error_exit; fi

echo "[3/4] Compiling codegen..."
clang compiler/codegen/codegen.c -c -o compiler/builded/codegen.o

if [ $? -ne 0 ]; then error_exit; fi

echo "[4/4] Linking NwC..."
clang compiler/compilerMain.c compiler/builded/lexer.o compiler/builded/parser.o compiler/builded/codegen.o -o compiler/nwc
if [ $? -ne 0 ]; then error_exit; fi

echo
echo "================================"
echo "       BUILD SUCCESS"
echo "================================"
echo

read -rp "Press Enter to continue..." _
exit 0
