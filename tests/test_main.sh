#!/bin/bash
source tests/test_helper.sh

check "引数なしでUsage表示" "Usage: ./ircserv <port> <password>" "$(./ircserv 2>&1)"
check "引数1つでUsage表示" "Usage: ./ircserv <port> <password>" "$(./ircserv 6667 2>&1)"
check "引数3つでUsage表示" "Usage: ./ircserv <port> <password>" "$(./ircserv 6667 pass extra 2>&1)"
check "正しい引数でhello" "hello" "$(./ircserv 6667 pass 2>&1)"

summary "test_main"