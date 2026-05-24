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