#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_user_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# USERパラメータ不足（4つ未満）→ ERR_NEEDMOREPARAMS(461)
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice\r\n' "$PASSWORD" >&3
resp=$(read_lines 3 1)
check "USERパラメータ不足でERR_NEEDMOREPARAMS(461)" "1" "$(echo "$resp" | grep -c ' 461 ')"
exec 3<&- 3>&-

# 正常な登録でWelcome(001)を受信
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
resp=$(read_lines 3 4)
check "正常登録でRPL_WELCOME(001)を受信" "1" "$(echo "$resp" | grep -c ' 001 ')"

# 登録後にUSER → ERR_ALREADYREGISTRED(462)
printf 'USER alice2 0 * :Alice2\r\n' >&3
resp=$(read_lines 3 1)
check "登録後のUSERでERR_ALREADYREGISTRED(462)" "1" "$(echo "$resp" | grep -c ' 462 ')"
exec 3<&- 3>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_user_out.txt

summary "test_user"
