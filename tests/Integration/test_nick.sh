#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_nick_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# NICKパラメータなし → ERR_NONICKNAMEGIVEN(431)
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK\r\n' "$PASSWORD" >&3
resp=$(read_lines 3 1)
check "NICKパラメータなしでERR_NONICKNAMEGIVEN(431)" "1" "$(echo "$resp" | grep -c ' 431 ')"
exec 3<&- 3>&-

# 数字で始まるニックネーム → ERR_ERRONEUSNICKNAME(432)
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK 1badnick\r\n' "$PASSWORD" >&3
resp=$(read_lines 3 1)
check "数字で始まるニックネームでERR_ERRONEUSNICKNAME(432)" "1" "$(echo "$resp" | grep -c ' 432 ')"
exec 3<&- 3>&-

# 10文字を超えるニックネーム → ERR_ERRONEUSNICKNAME(432)
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK toolongnick\r\n' "$PASSWORD" >&3
resp=$(read_lines 3 1)
check "10文字超のニックネームでERR_ERRONEUSNICKNAME(432)" "1" "$(echo "$resp" | grep -c ' 432 ')"
exec 3<&- 3>&-

# 使用中のニックネームを別クライアントが使用 → ERR_NICKNAMEINUSE(433)
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
read_lines 3 1 > /dev/null

exec 4<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\n' "$PASSWORD" >&4
resp=$(read_lines 4 1)
check "使用中のニックネームでERR_NICKNAMEINUSE(433)" "1" "$(echo "$resp" | grep -c ' 433 ')"
exec 4<&- 4>&-
exec 3<&- 3>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_nick_out.txt

summary "test_nick"
