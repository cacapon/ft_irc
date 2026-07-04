#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_kick_out.txt 2>&1 &
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

# KICKパラメータ不足（チャンネルのみ）→ ERR_NEEDMOREPARAMS(461)
printf 'KICK #test\r\n' >&3
resp=$(read_lines 3 1)
check "KICKパラメータ不足でERR_NEEDMOREPARAMS(461)" "1" "$(echo "$resp" | grep -c ' 461 ')"

# 存在しないチャンネルをKICK → ERR_NOSUCHCHANNEL(403)
printf 'KICK #nosuch bob\r\n' >&3
resp=$(read_lines 3 1)
check "存在しないチャンネルのKICKでERR_NOSUCHCHANNEL(403)" "1" "$(echo "$resp" | grep -c ' 403 ')"

# aliceが#testを作成（alice=オペレータ）
printf 'JOIN #test\r\n' >&3
read_lines 3 2 > /dev/null

# bobが#testに未参加でKICK → ERR_NOTONCHANNEL(442)
printf 'KICK #test alice\r\n' >&4
resp=$(read_lines 4 1)
check "未参加チャンネルでのKICKでERR_NOTONCHANNEL(442)" "1" "$(echo "$resp" | grep -c ' 442 ')"

# bobが#testに参加
printf 'JOIN #test\r\n' >&4
read_lines 4 2 > /dev/null
read_lines 3 1 > /dev/null  # aliceにbobのJOIN通知が届く、破棄

# bob（非オペレータ）がKICK → ERR_CHANOPRIVSNEEDED(482)
printf 'KICK #test alice\r\n' >&4
resp=$(read_lines 4 1)
check "非オペレータのKICKでERR_CHANOPRIVSNEEDED(482)" "1" "$(echo "$resp" | grep -c ' 482 ')"

# alice（オペレータ）が存在しないニックをKICK → ERR_NOSUCHNICK(401)
printf 'KICK #test nobody\r\n' >&3
resp=$(read_lines 3 1)
check "存在しないニックのKICKでERR_NOSUCHNICK(401)" "1" "$(echo "$resp" | grep -c ' 401 ')"

# charlie登録（#testに未参加）
exec 5<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK charlie\r\nUSER charlie 0 * :Charlie\r\n' "$PASSWORD" >&5
read_lines 5 4 > /dev/null

# alice（オペレータ）がcharlieをKICK（チャンネル未参加）→ ERR_USERNOTINCHANNEL(441)
printf 'KICK #test charlie\r\n' >&3
resp=$(read_lines 3 1)
check "チャンネル未参加ユーザーのKICKでERR_USERNOTINCHANNEL(441)" "1" "$(echo "$resp" | grep -c ' 441 ')"

# alice（オペレータ）がbobをKICK（理由付き）→ alice・bob両方がKICK通知を受信
printf 'KICK #test bob :bye\r\n' >&3
resp_alice=$(read_lines 3 1)
resp_bob=$(read_lines 4 1)
check "KICK成功でオペレータがKICK通知を受信" "1" \
    "$(echo "$resp_alice" | grep -c ':alice!alice@localhost KICK #test bob :bye')"
check "KICK成功でキックされたユーザーもKICK通知を受信" "1" \
    "$(echo "$resp_bob" | grep -c ':alice!alice@localhost KICK #test bob :bye')"

exec 3<&- 3>&-
exec 4<&- 4>&-
exec 5<&- 5>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_kick_out.txt

summary "test_kick"
