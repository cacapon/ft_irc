#!/bin/bash
# tests/test_main.sh

PASS=0
FAIL=0

# テスト関数
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

# テスト実行
check "hello返す" "hello" "$(./ircserv)"

# 結果
echo ""
echo "Result: $PASS passed, $FAIL failed"
[ $FAIL -eq 0 ] && exit 0 || exit 1