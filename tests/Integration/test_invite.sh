#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_invite_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# alice登録・#testに参加（alice=オペレータ）
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
read_lines 3 1 > /dev/null
printf 'JOIN #test\r\n' >&3
read_lines 3 2 > /dev/null

# bob登録
exec 4<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK bob\r\nUSER bob 0 * :Bob\r\n' "$PASSWORD" >&4
read_lines 4 1 > /dev/null

# INVITEパラメータ不足（ニックのみ）→ ERR_NEEDMOREPARAMS(461)
printf 'INVITE bob\r\n' >&3
resp=$(read_lines 3 1)
check "INVITEパラメータ不足でERR_NEEDMOREPARAMS(461)" "1" "$(echo "$resp" | grep -c ' 461 ')"

# 存在しないチャンネルへのINVITE → ERR_NOSUCHCHANNEL(403)
printf 'INVITE bob #nosuch\r\n' >&3
resp=$(read_lines 3 1)
check "存在しないチャンネルへのINVITEでERR_NOSUCHCHANNEL(403)" "1" "$(echo "$resp" | grep -c ' 403 ')"

# bobが参加していないチャンネルからINVITE → ERR_NOTONCHANNEL(442)
printf 'INVITE alice #test\r\n' >&4
resp=$(read_lines 4 1)
check "未参加チャンネルからのINVITEでERR_NOTONCHANNEL(442)" "1" "$(echo "$resp" | grep -c ' 442 ')"

# 存在しないニックをINVITE → ERR_NOSUCHNICK(401)
printf 'INVITE nobody #test\r\n' >&3
resp=$(read_lines 3 1)
check "存在しないニックへのINVITEでERR_NOSUCHNICK(401)" "1" "$(echo "$resp" | grep -c ' 401 ')"

# bobが#testに参加、aliceが再度INVITE → ERR_USERONCHANNEL(443)
printf 'JOIN #test\r\n' >&4
read_lines 4 2 > /dev/null
read_lines 3 1 > /dev/null  # aliceにbobのJOIN通知が届く、破棄

printf 'INVITE bob #test\r\n' >&3
resp=$(read_lines 3 1)
check "既にチャンネルにいるユーザーへのINVITEでERR_USERONCHANNEL(443)" "1" "$(echo "$resp" | grep -c ' 443 ')"

# invite-only チャンネルの非オペレータがINVITE → ERR_CHANOPRIVSNEEDED(482)
# aliceが#invitechanを作成してinvite-onlyに設定
printf 'JOIN #invitechan\r\n' >&3
read_lines 3 2 > /dev/null
printf 'MODE #invitechan +i\r\n' >&3
read_lines 3 1 > /dev/null

# bobが#invitechanに参加してINVITEを試みる（bobは非オペレータ）
printf 'JOIN #invitechan\r\n' >&4
resp=$(read_lines 4 1)  # bobは473を受信（invite-only）
check "invite-onlyチャンネルへの未招待JOINでERR_INVITEONLYCHAN(473)" "1" "$(echo "$resp" | grep -c ' 473 ')"

# aliceがbobを#invitechanに招待 → alice: RPL_INVITING(341)、bob: INVITE通知
printf 'INVITE bob #invitechan\r\n' >&3
resp_alice=$(read_lines 3 1)
resp_bob=$(read_lines 4 1)
check "INVITE成功でRPL_INVITING(341)を受信" "1" "$(echo "$resp_alice" | grep -c ' 341 ')"
check "招待されたユーザーがINVITE通知を受信" "1" \
    "$(echo "$resp_bob" | grep -c ':alice!alice@localhost INVITE bob #invitechan')"

# 招待されたbobがinvite-onlyチャンネルに参加できる
printf 'JOIN #invitechan\r\n' >&4
resp=$(read_lines 4 2)
check "招待後にinvite-onlyチャンネルへのJOINが成功" "1" "$(echo "$resp" | grep -c ' JOIN ')"
read_lines 3 1 > /dev/null  # aliceにbobのJOIN通知が届く、破棄

exec 3<&- 3>&-
exec 4<&- 4>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_invite_out.txt

summary "test_invite"
