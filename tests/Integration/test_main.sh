#!/bin/bash
source tests/Integration/test_helper.sh

check "引数なしでUsage表示" "Usage: ./ircserv <port> <password>" "$(./ircserv 2>&1)"
check "引数1つでUsage表示" "Usage: ./ircserv <port> <password>" "$(./ircserv 6667 2>&1)"
check "引数3つでUsage表示" "Usage: ./ircserv <port> <password>" "$(./ircserv 6667 pass extra 2>&1)"

# サーバー起動確認
./ircserv 6667 pass > /tmp/ircserv_out.txt &
SERVER_PID=$!
sleep 0.5

check "正しい引数でServer listening" \
	"Server listening on port 6667" \
	"$(cat /tmp/ircserv_out.txt)"

(echo > /dev/tcp/127.0.0.1/6667) 2>/dev/null
check "ポート6667でlistenしている" "0" "$?"


kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_out.txt

summary "test_main"