#!/bin/bash
# 素のnc(Enterは\nのみ送出)でも行が完成し登録できることのテスト。
# - \n単独終端で登録・PING応答できること
# - \r\n終端の従来経路が退行していないこと
# - \r\nと\n単独が混在しても各行が正しく分割されること
# - 1コマンドを複数writeに分割しても再構成されること
source tests/Integration/test_helper.sh

PORT=6670
PASSWORD=pass

./ircserv $PORT $PASSWORD > /tmp/ircserv_barelf_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

read_lines() {
    local fd=$1
    local n=$2
    local out=""
    local line
    local i=0
    while [ "$i" -lt "$n" ]; do
        read -t 5 -r line <&"$fd" || break
        out+="$line"$'\n'
        i=$((i + 1))
    done
    printf '%s' "$out"
}

drain() {
    local fd=$1
    local out=""
    local line
    while read -t 1 -r line <&"$fd"; do
        out+="$line"$'\n'
    done
    printf '%s' "$out"
}

# --- \n単独終端で登録できること ---
exec 3<>/dev/tcp/127.0.0.1/$PORT
printf 'PASS %s\nNICK alice\nUSER alice 0 * :Alice\n' "$PASSWORD" >&3
resp=$(read_lines 3 1)
check "\\n単独終端でWelcome(001)を受信" "1" "$(echo "$resp" | grep -c ' 001 ')"

printf 'PING hello\n' >&3
resp=$(read_lines 3 1)
check "\\n単独終端でPING応答を受信" "1" \
    "$(echo "$resp" | grep -c ':ircserv PONG ircserv :hello')"

# --- \r\n従来経路の退行確認 ---
printf 'PING again\r\n' >&3
resp=$(read_lines 3 1)
check "\\r\\n終端でも従来通りPING応答を受信" "1" \
    "$(echo "$resp" | grep -c ':ircserv PONG ircserv :again')"

# --- \r\nと\n単独の混在 ---
printf 'PING mix1\r\nPING mix2\n' >&3
resp=$(read_lines 3 2)
check "\\r\\nと\\n混在時に2行とも応答される" "2" \
    "$(echo "$resp" | grep -c ':ircserv PONG ircserv :mix')"

# --- 分割送信(複数writeに分けても再構成される) ---
printf 'PI' >&3
sleep 0.1
printf 'NG spl' >&3
sleep 0.1
printf 'it\n' >&3
resp=$(read_lines 3 1)
check "分割送信でも\\n到達時に再構成される" "1" \
    "$(echo "$resp" | grep -c ':ircserv PONG ircserv :split')"

exec 3<&- 3>&-

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_barelf_out.txt

summary "test_bare_lf"
