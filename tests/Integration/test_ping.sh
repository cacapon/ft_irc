#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_ping_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# 期待する行数分だけ読む（来なければ5秒で異常終了）
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

# alice接続・登録
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
resp=$(read_lines 3 1)
check "alice登録でWelcome(001)を受信" "1" "$(echo "$resp" | grep -c ' 001 ')"

printf 'PING hello\r\n' >&3
resp=$(read_lines 3 1)
check "PING送信後にメッセージを受信" "1" \
    "$(echo "$resp" | grep -c ':ircserv PONG ircserv :hello')"

exec 3<&- 3>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_ping_out.txt

summary "test_ping"
