#!/usr/bin/env bash
# ばか長文字列の耐久性テスト.
#
# サーバは receiveData で \r\n が来るまで recv バッファを無制限に伸ばす
# (Server.cpp)。行長上限を実装していないため、巨大な単一行でメモリを
# 食い潰せるか / サーバが生き残るかを確認する。
#
# 各ケースの後、サーバが「まだ生きていて新規接続を受け付けるか」を
# PING/PONG で検証する。生存できていれば PASS。
#
# 使い方: ./long_string.sh [host] [port] [password]
set -u

HOST="${1:-127.0.0.1}"
PORT="${2:-6667}"
PASS="${3:-pass}"

pass_cnt=0
fail_cnt=0

# サーバ生存確認: 新規接続で PING を送り PONG が返るか
alive_check() {
    local out
    out=$(printf 'PING :alive\r\n' | nc -w 2 "$HOST" "$PORT" 2>/dev/null)
    if printf '%s' "$out" | grep -q "PONG"; then
        return 0
    fi
    return 1
}

# 1行に size バイトの引数を持つ PRIVMSG 相当を送りつける
send_long_line() {
    local size="$1"
    local payload
    payload=$(head -c "$size" /dev/zero | tr '\0' 'A')
    # 登録してから巨大 PRIVMSG を1行で送る（CRLF は末尾のみ）
    {
        printf 'PASS %s\r\n' "$PASS"
        printf 'NICK longtester\r\n'
        printf 'USER lt 0 * :lt\r\n'
        printf 'PRIVMSG #x :%s\r\n' "$payload"
    } | nc -w 3 "$HOST" "$PORT" >/dev/null 2>&1
}

# CRLF を一切含まない巨大バイト列を流し込む（バッファ無制限増加の確認）
send_no_crlf() {
    local size="$1"
    head -c "$size" /dev/zero | tr '\0' 'A' | nc -w 3 "$HOST" "$PORT" >/dev/null 2>&1
}

rss_mb() {
    local pid
    pid=$(pgrep -f "ircserv $PORT" | head -1)
    [ -n "$pid" ] && ps -o rss= -p "$pid" 2>/dev/null | awk '{printf "%.0f", $1/1024}'
}

run_case() {
    local desc="$1"; shift
    printf '── %-32s ' "$desc"
    local t0 t1
    t0=$(python3 -c 'import time;print(time.time())')
    "$@"
    t1=$(python3 -c 'import time;print(time.time())')
    local dt
    dt=$(python3 -c "print(f'{$t1-$t0:.2f}')")
    if alive_check; then
        printf 'PASS  %6ss  RSS=%sMB\n' "$dt" "$(rss_mb)"
        pass_cnt=$((pass_cnt + 1))
    else
        printf 'FAIL  %6ss (dead/unresponsive)\n' "$dt"
        fail_cnt=$((fail_cnt + 1))
    fi
}

echo "対象: $HOST:$PORT   （処理時間の伸びに注目: 線形でなければ O(n^2)）"
echo

run_case "512B 1行 PRIVMSG"       send_long_line 512
run_case "64KB 1行 PRIVMSG"       send_long_line 65536
run_case "1MB 1行 PRIVMSG"        send_long_line 1048576
run_case "4MB 1行 PRIVMSG"        send_long_line 4194304
run_case "8MB 1行 PRIVMSG"        send_long_line 8388608
run_case "8MB CRLFなし流し込み"   send_no_crlf 8388608

echo
echo "PASS=$pass_cnt FAIL=$fail_cnt"
[ "$fail_cnt" -eq 0 ]
