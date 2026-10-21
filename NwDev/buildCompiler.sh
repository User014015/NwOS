#!/usr/bin/env bash

# ============================================================
# NwDev build script
#
# Expected layout:
#   NwOS/
#     compiler/
#       builded/nwc
#     NwDev/
#       buildCompiler.sh
#       *.nw
#       *.nwo       <- generated beside the .nw source
#
# Usage:
#   ./buildCompiler.sh              -> compile every *.nw in NwDev
#   ./buildCompiler.sh file.nw      -> compile only file.nw
#   ./buildCompiler.sh clean        -> delete generated *.nwo files
# ============================================================

cd "$(dirname "$0")" || exit 1

NwDEV_ROOT="$PWD"
NWC="$NwDEV_ROOT/../compiler/builded/nwc"


build_single() {
    INPUT="$1"

    if [ ! -f "$INPUT" ]; then
        echo
        echo "[NwDev] ERROR: source file not found:"
        echo "        $INPUT"
        echo
        exit 1
    fi

    INPUT_FULL="$(realpath "$INPUT")"
    OUTPUT="${INPUT%.*}.nwo"

    echo
    echo "========================================"
    echo "              NwDev compiler"
    echo "========================================"
    echo "Input : $INPUT_FULL"
    echo "Output: $OUTPUT"
    echo "========================================"
    echo

    "$NWC" "$INPUT_FULL" -o "$OUTPUT"
    RESULT=$?

    if [ "$RESULT" != "0" ]; then
        echo
        echo "[NwDev] Compilation FAILED."
        exit "$RESULT"
    fi

    echo
    echo "[NwDev] Compilation successful."
    echo

    exit 0
}


build_all() {
    TOTAL=0
    FAILED=0

    shopt -s nullglob
    SOURCES=(*.nw)
    shopt -u nullglob

    for F in "${SOURCES[@]}"; do
        TOTAL=$((TOTAL + 1))
        INPUT="$(realpath "$F")"
        OUTPUT="${F%.*}.nwo"

        echo
        echo "----------------------------------------"
        echo "Compiling: $F"
        echo "----------------------------------------"

        "$NWC" "$INPUT" -o "$OUTPUT"

        if [ $? -ne 0 ]; then
            FAILED=$((FAILED + 1))
            echo "[NwDev] FAILED: $F"
        else
            echo "[NwDev] OK: $F"
        fi
    done

    if [ "$TOTAL" == "0" ]; then
        echo
        echo "[NwDev] No .nw files found in:"
        echo "        $NwDEV_ROOT"
        echo
        exit 1
    fi

    echo
    echo "========================================"
    echo "              NwDev summary"
    echo "========================================"
    echo "Total : $TOTAL"
    echo "Failed: $FAILED"
    echo "========================================"
    echo

    if [ "$FAILED" -gt 0 ]; then
        exit 1
    fi

    exit 0
}


clean_all() {
    echo
    echo "========================================"
    echo "             Cleaning NwDev"
    echo "========================================"

    REMOVED=0

    shopt -s nullglob
    OUTPUTS=(*.nwo)
    shopt -u nullglob

    for F in "${OUTPUTS[@]}"; do
        rm -f "$F"
        if [ ! -f "$F" ]; then REMOVED=$((REMOVED + 1)); fi
    done

    echo
    echo "Removed: $REMOVED .nwo file(s)."
    echo

    exit 0
}


if [ ! -f "$NWC" ]; then
    echo
    echo "[NwDev] ERROR: NwC compiler was not found:"
    echo "        $NWC"
    echo
    echo "Build the compiler first:"
    echo "        ../compiler/build.sh"
    echo
    exit 1
fi

if [ "${1,,}" = "clean" ]; then
    clean_all
fi

if [ -n "$1" ]; then
    build_single "$1"
fi

build_all
