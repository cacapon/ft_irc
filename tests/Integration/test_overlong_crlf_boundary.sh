#!/bin/bash
# issue #70: 超長行の終端CRLFがパケット境界で分割されたとき、直後の正当な
# コマンドを失わないことのテスト（bash /dev/tcp では write 単位を厳密に
# 制御できないため、python3 の socket で send を分割する）。
# - 超長行の \r\n が \r と \n に分割されて届いても直後のコマンドが失われない
# - \r\n の手前（本文とCRLFの間）で分割されても直後のコマンドが失われない
# - \r\n と直後コマンドの間がさらに分割されても失われない
source tests/Integration/test_helper.sh

PORT=6671
PASSWORD=pass

if ! command -v python3 >/dev/null 2>&1; then
    echo "SKIP: python3 not found"
    summary "test_overlong_crlf_boundary"
fi

./ircserv $PORT $PASSWORD > /tmp/ircserv_overlong_crlf_out.txt 2>&1 &
SERVER_PID=$!
sleep 0.3

# 引数: NICK に続けて送信チャンクの16進表現。各チャンクをsend後15ms待ってから
# 次を送り、パケット境界を分割する。最後にPONGが返れば直後コマンドが失われていない。
run_case() {
    python3 - "$PORT" "$PASSWORD" "$@" << 'PYEOF'
import socket
import sys
import time

port = int(sys.argv[1])
password = sys.argv[2]
nick = sys.argv[3]
chunks = [bytes.fromhex(h) for h in sys.argv[4:]]

s = socket.create_connection(("127.0.0.1", port))
s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
s.sendall(("PASS %s\r\nNICK %s\r\nUSER %s 0 * :T\r\n" % (password, nick, nick)).encode())


def recv_all(timeout):
    s.settimeout(timeout)
    data = []
    try:
        while True:
            chunk = s.recv(65536)
            if not chunk:
                break
            data.append(chunk)
    except socket.timeout:
        pass
    return b"".join(data)


welcome = recv_all(1.0)
if b" 001 " not in welcome:
    print("no_welcome")
    sys.exit(0)

for chunk in chunks:
    s.sendall(chunk)
    time.sleep(0.15)

resp = recv_all(1.5)
s.close()
print("pong" if b"PONG ircserv :boundary" in resp else "no_pong")
PYEOF
}

overlong_hex=$(python3 -c "print(('PRIVMSG #x :' + 'A' * 600).encode().hex())")
crlf_hex=$(printf '\r\n' | xxd -p | tr -d '\n')
cr_hex=$(printf '\r' | xxd -p | tr -d '\n')
ping1_hex=$(printf '\nPING boundary\r\n' | xxd -p | tr -d '\n')
ping2_hex=$(printf '\r\nPING boundary\r\n' | xxd -p | tr -d '\n')
ping_head_hex=$(printf 'PING' | xxd -p | tr -d '\n')
ping_tail_hex=$(printf ' boundary\r\n' | xxd -p | tr -d '\n')

# --- 超長行の \r と \n がパケット境界で分割される ---
result=$(run_case "bnda" "${overlong_hex}${cr_hex}" "$ping1_hex")
check "\\r|\\n分割後もPONGが返る(直後コマンドを失わない)" "pong" "$result"

# --- 本文と \r\n の間で分割される ---
result=$(run_case "bndb" "$overlong_hex" "$ping2_hex")
check "本文と\\r\\nの間で分割後もPONGが返る" "pong" "$result"

# --- \r\n と直後コマンドの間がさらに分割される ---
result=$(run_case "bndc" "${overlong_hex}${crlf_hex}" "$ping_head_hex" "$ping_tail_hex")
check "\\r\\n直後で更に分割されてもPONGが返る" "pong" "$result"

kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
rm -f /tmp/ircserv_overlong_crlf_out.txt

summary "test_overlong_crlf_boundary"
