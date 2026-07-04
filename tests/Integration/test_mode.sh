#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_mode_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# alice登録・#testに参加（alice=オペレータ）
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
read_lines 3 1 > /dev/null
printf 'JOIN #test\r\n' >&3
read_lines 3 2 > /dev/null

# MODEパラメータなし → ERR_NEEDMOREPARAMS(461)
printf 'MODE\r\n' >&3
resp=$(read_lines 3 1)
check "MODEパラメータなしでERR_NEEDMOREPARAMS(461)" "1" "$(echo "$resp" | grep -c ' 461 ')"

# 存在しないチャンネルへのMODE → ERR_NOSUCHCHANNEL(403)
printf 'MODE #nosuch\r\n' >&3
resp=$(read_lines 3 1)
check "存在しないチャンネルのMODEでERR_NOSUCHCHANNEL(403)" "1" "$(echo "$resp" | grep -c ' 403 ')"

# MODEクエリ（モード文字列なし）→ RPL_CHANNELMODEIS(324)
printf 'MODE #test\r\n' >&3
resp=$(read_lines 3 1)
check "MODEクエリでRPL_CHANNELMODEIS(324)を受信" "1" "$(echo "$resp" | grep -c ' 324 ')"

# bob登録・#testに参加（bob=非オペレータ）
exec 4<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK bob\r\nUSER bob 0 * :Bob\r\n' "$PASSWORD" >&4
read_lines 4 1 > /dev/null
printf 'JOIN #test\r\n' >&4
read_lines 4 2 > /dev/null
read_lines 3 1 > /dev/null  # aliceにbobのJOIN通知が届く、破棄

# bob（非オペレータ）がMODE設定 → ERR_CHANOPRIVSNEEDED(482)
printf 'MODE #test +i\r\n' >&4
resp=$(read_lines 4 1)
check "非オペレータのMODE設定でERR_CHANOPRIVSNEEDED(482)" "1" "$(echo "$resp" | grep -c ' 482 ')"

# +i モード設定 → MODE通知をチャンネル全員が受信
printf 'MODE #test +i\r\n' >&3
resp=$(read_lines 3 1)
check "+iモード設定でMODE通知を受信" "1" "$(echo "$resp" | grep -c '+i')"
read_lines 4 1 > /dev/null  # bobにもMODE通知が届く、破棄

# +t モード設定 → MODE通知を受信
printf 'MODE #test +t\r\n' >&3
resp=$(read_lines 3 1)
check "+tモード設定でMODE通知を受信" "1" "$(echo "$resp" | grep -c '+t')"
read_lines 4 1 > /dev/null

# +k モード設定 → MODE通知（キー付き）を受信
printf 'MODE #test +k secret\r\n' >&3
resp=$(read_lines 3 1)
check "+kモード設定でMODE通知を受信" "1" "$(echo "$resp" | grep -c '+k')"
read_lines 4 1 > /dev/null

# -k モード解除 → MODE通知を受信
printf 'MODE #test -k\r\n' >&3
resp=$(read_lines 3 1)
check "-kモード解除でMODE通知を受信" "1" "$(echo "$resp" | grep -c '\-k')"
read_lines 4 1 > /dev/null

# +l モード設定 → MODE通知（上限付き）を受信
printf 'MODE #test +l 5\r\n' >&3
resp=$(read_lines 3 1)
check "+lモード設定でMODE通知を受信" "1" "$(echo "$resp" | grep -c '+l')"
read_lines 4 1 > /dev/null

# -l モード解除 → MODE通知を受信
printf 'MODE #test -l\r\n' >&3
resp=$(read_lines 3 1)
check "-lモード解除でMODE通知を受信" "1" "$(echo "$resp" | grep -c '\-l')"
read_lines 4 1 > /dev/null

# +o でbobにオペレータ権限付与 → MODE通知を受信
printf 'MODE #test +o bob\r\n' >&3
resp=$(read_lines 3 1)
check "+oでオペレータ権限付与のMODE通知を受信" "1" "$(echo "$resp" | grep -c '+o')"
read_lines 4 1 > /dev/null

# 不正なモード文字 → ERR_UNKNOWNMODE(472)
printf 'MODE #test +z\r\n' >&3
resp=$(read_lines 3 1)
check "不正なモード文字でERR_UNKNOWNMODE(472)" "1" "$(echo "$resp" | grep -c ' 472 ')"

exec 3<&- 3>&-
exec 4<&- 4>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_mode_out.txt

summary "test_mode"
