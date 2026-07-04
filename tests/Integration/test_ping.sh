#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_ping_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# alice接続・登録
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
# 登録完了時は001-004の4行が送られる
resp=$(read_lines 3 4)
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
