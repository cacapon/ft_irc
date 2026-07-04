#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_pass_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# PASSパラメータなし → ERR_NEEDMOREPARAMS(461)
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS\r\n' >&3
resp=$(read_lines 3 1)
check "PASSパラメータなしでERR_NEEDMOREPARAMS(461)" "1" "$(echo "$resp" | grep -c ' 461 ')"
exec 3<&- 3>&-

# 間違ったパスワード → ERR_PASSWDMISMATCH(464)
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS wrongpass\r\n' >&3
resp=$(read_lines 3 1)
check "間違ったパスワードでERR_PASSWDMISMATCH(464)" "1" "$(echo "$resp" | grep -c ' 464 ')"
exec 3<&- 3>&-

# 登録後にPASS → ERR_ALREADYREGISTRED(462)
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
read_lines 3 4 > /dev/null
printf 'PASS %s\r\n' "$PASSWORD" >&3
resp=$(read_lines 3 1)
check "登録後のPASSでERR_ALREADYREGISTRED(462)" "1" "$(echo "$resp" | grep -c ' 462 ')"
exec 3<&- 3>&-

# PASS前のNICK → ERR_NOTREGISTERED(451)
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'NICK bob\r\n' >&3
resp=$(read_lines 3 1)
check "PASS前のNICKでERR_NOTREGISTERED(451)" "1" "$(echo "$resp" | grep -c ' 451 ')"
exec 3<&- 3>&-

# PASS前のPING → ERR_NOTREGISTERED(451)
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PING token\r\n' >&3
resp=$(read_lines 3 1)
check "PASS前のPINGでERR_NOTREGISTERED(451)" "1" "$(echo "$resp" | grep -c ' 451 ')"
exec 3<&- 3>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_pass_out.txt

summary "test_pass"
