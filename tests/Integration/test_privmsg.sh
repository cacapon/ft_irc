#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_privmsg_out.txt 2>&1 &
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

# PRIVMSGで宛先なし → ERR_NORECIPIENT(411)
printf 'PRIVMSG\r\n' >&3
resp=$(read_lines 3 1)
check "PRIVMSGで宛先なしでERR_NORECIPIENT(411)" "1" "$(echo "$resp" | grep -c ' 411 ')"

# PRIVMSGでテキストなし → ERR_NOTEXTTOSEND(412)
printf 'PRIVMSG bob\r\n' >&3
resp=$(read_lines 3 1)
check "PRIVMSGでテキストなしでERR_NOTEXTTOSEND(412)" "1" "$(echo "$resp" | grep -c ' 412 ')"

# PRIVMSGで存在しないニック → ERR_NOSUCHNICK(401)
printf 'PRIVMSG nobody :hello\r\n' >&3
resp=$(read_lines 3 1)
check "PRIVMSGで存在しないニックでERR_NOSUCHNICK(401)" "1" "$(echo "$resp" | grep -c ' 401 ')"

# PRIVMSGで参加していないチャンネル → ERR_CANNOTSENDTOCHAN(404)
# bobが#testチャンネルを作成、aliceは参加せずにメッセージ送信
printf 'JOIN #test\r\n' >&4
read_lines 4 4 > /dev/null  # JOIN + NOTOPIC + NAMES(353) + ENDOFNAMES(366)
printf 'PRIVMSG #test :hello\r\n' >&3
resp=$(read_lines 3 1)
check "参加していないチャンネルへのPRIVMSGでERR_CANNOTSENDTOCHAN(404)" "1" "$(echo "$resp" | grep -c ' 404 ')"

# PRIVMSGでチャンネルへのメッセージ（送信者以外が受信）
printf 'JOIN #test\r\n' >&3
resp=$(read_lines 3 4)  # alice: JOIN + NOTOPIC + NAMES(353) + ENDOFNAMES(366)
read_lines 4 1 > /dev/null  # bobにaliceのJOIN通知が届く、破棄

printf 'PRIVMSG #test :hello channel\r\n' >&3
resp=$(read_lines 4 1)  # bobがメッセージを受信
check "チャンネルへのPRIVMSGをメンバーが受信" "1" \
    "$(echo "$resp" | grep -c ':alice!alice@localhost PRIVMSG #test :hello channel')"

# PRIVMSGでDM（ニック宛て直接送信）
printf 'PRIVMSG alice :hi alice\r\n' >&4
resp=$(read_lines 3 1)  # aliceがDMを受信
check "ニック宛てPRIVMSGをDM受信" "1" \
    "$(echo "$resp" | grep -c ':bob!bob@localhost PRIVMSG alice :hi alice')"

exec 3<&- 3>&-
exec 4<&- 4>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_privmsg_out.txt

summary "test_privmsg"
