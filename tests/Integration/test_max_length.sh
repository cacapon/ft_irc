#!/bin/bash
# RFC 2812 の行長上限（512 = CRLF込み、内容510）のテスト。
# - 超長行の後もサーバが生存し再同期できること
# - CRLFなしの大量送信でも再同期できること（メモリDoS防止の副次確認）
# - 超長メッセージが 510 で truncate されること
source tests/Integration/test_helper.sh

PORT=6669
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_maxlen_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# 無音（1秒）になるまで到達している全行を集める。
# 応答行数が可変（エラー応答が混ざる等）のケースで使う。
drain() {
    local fd=$1
    local out=""
    local line
    while read -t 1 -r line <&"$fd"; do
        out+="$line"$'\n'
    done
    printf '%s' "$out"
}

# 指定バイト数の 'A' を生成
repeat() {
    head -c "$1" /dev/zero | tr '\0' 'A'
}

# --- alice 接続・登録 ---
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
# 登録完了時は001-004の4行が送られる
resp=$(read_lines 3 4)
check "alice登録でWelcome(001)を受信" "1" "$(echo "$resp" | grep -c ' 001 ')"

# --- 超長行(600文字)の直後に PING → 生存＆再同期の確認 ---
printf 'PRIVMSG #x :%s\r\n' "$(repeat 600)" >&3
printf 'PING :after-long\r\n' >&3
resp=$(drain 3)
check "超長行の後もPONGが返る(生存・再同期)" "1" \
    "$(echo "$resp" | grep -c ':ircserv PONG ircserv :after-long')"

# --- CRLFなしで大量送信 → CRLF後の PING が通る(バッファ上限で遮断・再同期) ---
repeat 5000 >&3
printf '\r\nPING :resync\r\n' >&3
resp=$(drain 3)
check "CRLFなし大量送信後もPONGが返る(再同期)" "1" \
    "$(echo "$resp" | grep -c ':ircserv PONG ircserv :resync')"
exec 3<&- 3>&-

# --- truncate 確認: 700文字本文の PRIVMSG が 510 で切られて中継される ---
exec 4<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK bob\r\nUSER bob 0 * :Bob\r\n' "$PASSWORD" >&4
read_lines 4 4 >/dev/null
printf 'JOIN #room\r\n' >&4
read_lines 4 1 >/dev/null

exec 5<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK carol\r\nUSER carol 0 * :Carol\r\n' "$PASSWORD" >&5
read_lines 5 4 >/dev/null
printf 'JOIN #room\r\n' >&5
read_lines 5 1 >/dev/null

# carol が本文700文字を送信 → bob が受信する行の 'Z' 個数が 510 未満なら truncate 成功
printf 'PRIVMSG #room :%s\r\n' "$(head -c 700 /dev/zero | tr '\0' 'Z')" >&5
resp=$(drain 4)
zcount=$(echo "$resp" | tr -cd 'Z' | wc -c | tr -d ' ')
check "700文字本文が truncate される(Z<510かつ>0)" "1" \
    "$([ "$zcount" -gt 0 ] && [ "$zcount" -lt 510 ] && echo 1 || echo 0)"

exec 4<&- 4>&-
exec 5<&- 5>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_maxlen_out.txt

summary "test_max_length"
