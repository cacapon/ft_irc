#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_join_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# alice登録
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
read_lines 3 4 > /dev/null

# JOINパラメータなし → ERR_NEEDMOREPARAMS(461)
printf 'JOIN\r\n' >&3
resp=$(read_lines 3 1)
check "JOINパラメータなしでERR_NEEDMOREPARAMS(461)" "1" "$(echo "$resp" | grep -c ' 461 ')"

# 不正なチャンネル名（#なし）→ ERR_NOSUCHCHANNEL(403)
printf 'JOIN badchan\r\n' >&3
resp=$(read_lines 3 1)
check "不正なチャンネル名でERR_NOSUCHCHANNEL(403)" "1" "$(echo "$resp" | grep -c ' 403 ')"

# 正常JOIN（新規チャンネル）→ JOIN通知 + RPL_NOTOPIC(331) + RPL_NAMREPLY(353) + RPL_ENDOFNAMES(366)
printf 'JOIN #test\r\n' >&3
resp=$(read_lines 3 4)
check "正常JOINでJOIN通知を受信" "1" "$(echo "$resp" | grep -c ' JOIN ')"
check "JOIN後にRPL_NOTOPIC(331)を受信" "1" "$(echo "$resp" | grep -c ' 331 ')"
check "JOIN後にRPL_NAMREPLY(353)で自分のみ@付きを受信" "1" \
    "$(echo "$resp" | grep -c ' 353 alice = #test :@alice')"
check "JOIN後にRPL_ENDOFNAMES(366)を受信" "1" "$(echo "$resp" | grep -c ' 366 ')"

# bob登録
exec 4<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK bob\r\nUSER bob 0 * :Bob\r\n' "$PASSWORD" >&4
read_lines 4 4 > /dev/null

# invite-onlyチャンネルテスト
# aliceが#inviteチャンネルを作成してinvite-onlyに設定
printf 'JOIN #invite\r\n' >&3
read_lines 3 4 > /dev/null  # JOIN + NOTOPIC + NAMES(353) + ENDOFNAMES(366)
printf 'MODE #invite +i\r\n' >&3
read_lines 3 1 > /dev/null

# bobがinvite-onlyチャンネルに参加しようとする → ERR_INVITEONLYCHAN(473)
printf 'JOIN #invite\r\n' >&4
resp=$(read_lines 4 1)
check "invite-onlyチャンネルへのJOINでERR_INVITEONLYCHAN(473)" "1" "$(echo "$resp" | grep -c ' 473 ')"

# キー付きチャンネルテスト
# aliceが#keychanを作成してキーを設定
printf 'JOIN #keychan\r\n' >&3
read_lines 3 4 > /dev/null  # JOIN + NOTOPIC + NAMES(353) + ENDOFNAMES(366)
printf 'MODE #keychan +k secret\r\n' >&3
read_lines 3 1 > /dev/null

# bobがキーなしで参加 → ERR_BADCHANNELKEY(475)
printf 'JOIN #keychan\r\n' >&4
resp=$(read_lines 4 1)
check "キーなしでJOINするとERR_BADCHANNELKEY(475)" "1" "$(echo "$resp" | grep -c ' 475 ')"

# bobが間違ったキーで参加 → ERR_BADCHANNELKEY(475)
printf 'JOIN #keychan wrongkey\r\n' >&4
resp=$(read_lines 4 1)
check "間違ったキーでJOINするとERR_BADCHANNELKEY(475)" "1" "$(echo "$resp" | grep -c ' 475 ')"

# bobが正しいキーで参加 → 成功（既存メンバーaliceを含むNAMESリストを受信）
printf 'JOIN #keychan secret\r\n' >&4
resp=$(read_lines 4 4)
check "正しいキーでJOIN成功" "1" "$(echo "$resp" | grep -c ' JOIN ')"
check "既存チャンネルJOINでRPL_NAMREPLY(353)に@付きオペレータと自分を受信" "1" \
    "$(echo "$resp" | grep -c ' 353 bob = #keychan :@alice bob')"
read_lines 3 1 > /dev/null  # aliceにbobのJOIN通知が届く、破棄

# 人数上限チャンネルテスト
# aliceが#fullチャンネルを作成して上限を1に設定
printf 'JOIN #full\r\n' >&3
read_lines 3 4 > /dev/null  # JOIN + NOTOPIC + NAMES(353) + ENDOFNAMES(366)
printf 'MODE #full +l 1\r\n' >&3
read_lines 3 1 > /dev/null

# bobが上限に達したチャンネルに参加 → ERR_CHANNELISFULL(471)
printf 'JOIN #full\r\n' >&4
resp=$(read_lines 4 1)
check "上限チャンネルへのJOINでERR_CHANNELISFULL(471)" "1" "$(echo "$resp" | grep -c ' 471 ')"

exec 3<&- 3>&-
exec 4<&- 4>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_join_out.txt

summary "test_join"
