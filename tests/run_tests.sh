#!/bin/bash
source tests/Integration/test_helper.sh

TOTAL_FAIL=0

run() {
    local script=$1
    bash $script
    if [ $? -ne 0 ]; then
        TOTAL_FAIL=$((TOTAL_FAIL + 1))
    fi
    echo ""
}

# コンパイル確認
make re > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "FAIL: コンパイルエラー"
    exit 1
fi
echo "OK: コンパイル"
echo ""

# 単体テスト実行
make unit_test
if [ $? -ne 0 ]; then
    TOTAL_FAIL=$((TOTAL_FAIL + 1))
fi
echo ""

# 統合テスト実行
run tests/Integration/test_main.sh
run tests/Integration/test_pass.sh
run tests/Integration/test_nick.sh
run tests/Integration/test_user.sh
run tests/Integration/test_join.sh
run tests/Integration/test_privmsg.sh
run tests/Integration/test_part.sh
run tests/Integration/test_mode.sh
run tests/Integration/test_kick.sh
run tests/Integration/test_invite.sh
run tests/Integration/test_topic.sh
run tests/Integration/test_ping.sh
run tests/Integration/test_max_length.sh
run tests/Integration/test_bare_lf.sh
run tests/Integration/test_overlong_crlf_boundary.sh

# 総合結果
echo "================================"
[ $TOTAL_FAIL -eq 0 ] && echo "All tests passed!" || echo "$TOTAL_FAIL test file(s) failed!"
[ $TOTAL_FAIL -eq 0 ] && exit 0 || exit 1