#!/bin/bash
source tests/Integration/test_helper.sh

PORT=6668
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_topic_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# fdに溜まっているレスポンスをタイムアウトまで読み切る
read_lines() {
    local fd=$1
    local timeout=${2:-1}
    local out=""
    local line
    while IFS= read -t "$timeout" -r line <&"$fd"; do
        out+="$line"$'\n'
    done
    printf '%s' "$out"
}

# alice接続・登録
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK alice\r\nUSER alice 0 * :Alice\r\n' "$PASSWORD" >&3
resp=$(read_lines 3)
check "alice登録でWelcome(001)を受信" "1" "$(echo "$resp" | grep -c ' 001 ')"

# JOIN（トピック未設定）
printf 'JOIN #test\r\n' >&3
resp=$(read_lines 3)
check "JOIN直後にRPL_NOTOPIC(331)を受信" "1" \
    "$(echo "$resp" | grep -c ':ircserv 331 alice #test :No topic is set')"

# TOPIC参照（未設定）
printf 'TOPIC #test\r\n' >&3
resp=$(read_lines 3)
check "トピック未設定時の参照でRPL_NOTOPIC(331)" "1" \
    "$(echo "$resp" | grep -c ':ircserv 331 alice #test :No topic is set')"

# TOPIC設定
printf 'TOPIC #test :Hello World\r\n' >&3
resp=$(read_lines 3)
check "トピック設定時に変更通知を受信" "1" \
    "$(echo "$resp" | grep -c ':alice!alice@localhost TOPIC #test :Hello World')"

# TOPIC参照（設定後）
printf 'TOPIC #test\r\n' >&3
resp=$(read_lines 3)
check "トピック設定後の参照でRPL_TOPIC(332)" "1" \
    "$(echo "$resp" | grep -c ':ircserv 332 alice #test :Hello World')"

# パラメータ不足
printf 'TOPIC\r\n' >&3
resp=$(read_lines 3)
check "パラメータ不足でERR_NEEDMOREPARAMS(461)" "1" "$(echo "$resp" | grep -c ' 461 ')"

# 存在しないチャンネル
printf 'TOPIC #nosuch\r\n' >&3
resp=$(read_lines 3)
check "存在しないチャンネルでERR_NOSUCHCHANNEL(403)" "1" "$(echo "$resp" | grep -c ' 403 ')"

# bob接続・登録（#testには未参加）
exec 4<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\r\nNICK bob\r\nUSER bob 0 * :Bob\r\n' "$PASSWORD" >&4
read_lines 4 > /dev/null

printf 'TOPIC #test\r\n' >&4
resp=$(read_lines 4)
check "未参加チャンネルへのTOPICでERR_NOTONCHANNEL(442)" "1" "$(echo "$resp" | grep -c ' 442 ')"

exec 3<&- 3>&-
exec 4<&- 4>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_topic_out.txt

summary "test_topic"
