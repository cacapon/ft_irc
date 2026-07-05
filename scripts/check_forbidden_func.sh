#!/bin/bash

ALLOWED_SYSCALLS=(
  "socket" "close" "setsockopt" "getsockname"
  "getprotobyname" "gethostbyname" "getaddrinfo" "freeaddrinfo"
  "bind" "connect" "listen" "accept"
  "htons" "htonl" "ntohs" "ntohl"
  "inet_addr" "inet_ntoa" "inet_ntop"
  "send" "recv" "lseek" "fstat" "poll"
  "signal" "sigaction" "sigemptyset" "sigfillset"
  "sigaddset" "sigdelset" "sigismember"

  # C++98標準ライブラリ
  "atoi" "strlen" "isdigit"
  "isalnum" "isalpha" "toupper"
  "memcmp" "memmove" "memset"

  # C++ランタイム
  "Unwind_Resume"
)

# macOSならfcntlを追加
if [[ "$(uname)" == "Darwin" ]]; then
  ALLOWED_SYSCALLS+=("fcntl")
fi

FAILED=0

make 2>/dev/null
if [ $? -ne 0 ]; then
  echo "❌ ビルド失敗"
  exit 1
fi

# distを除外、UNDEFINEDシンボルのみ、マングリング解除
USED=$(nm --undefined-only $(find ./objs -name "*.o") 2>/dev/null \
  | grep -v '^\.' \
  | awk '{print $NF}' \
  | c++filt \
  | grep -v '^std::' \
  | grep -v '^__' \
  | grep -v '::' \
  | grep -v '^operator' \
  | grep -v '^ft_' \
  | sed 's/^_//' \
  | sort -u)

ALLOWED_PATTERN=$(IFS='|'; echo "${ALLOWED_SYSCALLS[*]}")
FORBIDDEN=$(echo "$USED" | grep -Ev "^($ALLOWED_PATTERN)$")

if [ -n "$FORBIDDEN" ]; then
  echo "❌ 許可外のシンボルが含まれています:"
  echo "$FORBIDDEN"
  FAILED=1
fi

if grep -r "boost/" srcs/ incs/ 2>/dev/null; then
  echo "❌ Boost使用禁止"
  FAILED=1
fi

[ $FAILED -eq 1 ] && exit 1
echo "✅ 全チェック通過"
exit 0