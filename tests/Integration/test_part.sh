#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_part_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# alice登録
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
read_lines 3 4 > /dev/null

# bob登録
exec 4<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK bob\r\nUSER bob 0 * :Bob\r\n' "$PASSWORD" >&4
read_lines 4 4 > /dev/null

# PARTパラメータなし → ERR_NEEDMOREPARAMS(461)
printf 'PART\r\n' >&3
resp=$(read_lines 3 1)
check "PARTパラメータなしでERR_NEEDMOREPARAMS(461)" "1" "$(echo "$resp" | grep -c ' 461 ')"

# 存在しないチャンネルをPART → ERR_NOSUCHCHANNEL(403)
printf 'PART #nosuch\r\n' >&3
resp=$(read_lines 3 1)
check "存在しないチャンネルのPARTでERR_NOSUCHCHANNEL(403)" "1" "$(echo "$resp" | grep -c ' 403 ')"

# bobが#testを作成、aliceは未参加でPART → ERR_NOTONCHANNEL(442)
printf 'JOIN #test\r\n' >&4
read_lines 4 4 > /dev/null  # JOIN + NOTOPIC + NAMES(353) + ENDOFNAMES(366)
printf 'PART #test\r\n' >&3
resp=$(read_lines 3 1)
check "参加していないチャンネルのPARTでERR_NOTONCHANNEL(442)" "1" "$(echo "$resp" | grep -c ' 442 ')"

# aliceが#testに参加してPART → PART通知を受信
printf 'JOIN #test\r\n' >&3
read_lines 3 4 > /dev/null  # alice: JOIN + NOTOPIC + NAMES(353) + ENDOFNAMES(366)
read_lines 4 1 > /dev/null  # bobにaliceのJOIN通知が届く、破棄

printf 'PART #test\r\n' >&3
resp=$(read_lines 3 1)
check "正常PARTでPART通知を受信" "1" "$(echo "$resp" | grep -c 'PART #test')"
read_lines 4 1 > /dev/null  # bobにaliceのPART通知が届く、破棄

# 理由付きPART → 理由付きPART通知を受信
printf 'JOIN #test\r\n' >&3
read_lines 3 4 > /dev/null  # JOIN + NOTOPIC + NAMES(353) + ENDOFNAMES(366)
read_lines 4 1 > /dev/null

printf 'PART #test :goodbye\r\n' >&3
resp=$(read_lines 3 1)
check "理由付きPARTで理由付きPART通知を受信" "1" \
    "$(echo "$resp" | grep -c 'PART #test :goodbye')"
read_lines 4 1 > /dev/null

exec 3<&- 3>&-
exec 4<&- 4>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_part_out.txt

summary "test_part"
