#!/bin/bash

PASS=0
FAIL=0

check() {
    local description=$1
    local expected=$2
    local actual=$3

    if [ "$actual" = "$expected" ]; then
        echo "OK: $description"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $description"
        echo "  expected: $expected"
        echo "  actual:   $actual"
        FAIL=$((FAIL + 1))
    fi
}

summary() {
    local name=$1
    echo ""
    echo "=== $name: $PASS passed, $FAIL failed ==="
    [ $FAIL -eq 0 ] && exit 0 || exit 1
}

read_lines() {
    local fd=$1
    local n=$2
    local out=""
    local line
    local i=0
    while [ "$i" -lt "$n" ]; do
        read -t 5 -r line <&"$fd" || break
        out+="$line"$'\n'
        i=$((i + 1))
    done
    printf '%s' "$out"
}