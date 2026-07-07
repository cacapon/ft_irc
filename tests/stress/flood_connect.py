#!/usr/bin/env python3
"""大人数（万単位）同時接続の耐久性テスト.

単一プロセスで asyncio により N 本の TCP 接続を張り、各接続で
PASS / NICK / USER を送って 001 (welcome) を待つ。接続を保持したまま
成功数・失敗理由・所要時間・サーバの RSS を報告する。

nc は 1 接続 = 1 プロセスで数万に耐えられないため、大人数テストは本スクリプトを使う。
（機能テスト・ばか長文字列テストは nc / irssi で行う）

使い方:
    python3 flood_connect.py --host 127.0.0.1 --port 6667 --pass pass \
        --count 15000 --concurrency 500 --hold 3

終了コード: 目標数の 99% 以上が登録成功なら 0、それ未満なら 1。
"""
import argparse
import asyncio
import subprocess
import time


def nick_for(i):
    """9 文字以内・有効文字のみのユニークな nick を生成する。"""
    base = "0123456789abcdefghijklmnopqrstuvwxyz"
    s = ""
    n = i
    while True:
        s = base[n % 36] + s
        n //= 36
        if n == 0:
            break
    return ("n" + s)[:9]


async def one_client(i, host, port, password, hold, stats, sem):
    # セマフォは「接続確立＋登録」フェーズだけを律速する（backlog あふれ防止）。
    # hold 中は解放するので、確立済み接続は count 本まで積み上がり、
    # 真の同時接続数を測れる。
    writer = None
    try:
        async with sem:
            reader, writer = await asyncio.wait_for(
                asyncio.open_connection(host, port), timeout=30
            )
            stats["connected"] += 1
            nick = nick_for(i)
            reg = f"PASS {password}\r\nNICK {nick}\r\nUSER {nick} 0 * :{nick}\r\n"
            writer.write(reg.encode())
            await writer.drain()

            # 001 (welcome) を待つ
            got_welcome = False
            buf = b""
            while True:
                try:
                    chunk = await asyncio.wait_for(reader.read(4096), timeout=30)
                except asyncio.TimeoutError:
                    break
                if not chunk:
                    break
                buf += chunk
                if b" 001 " in buf:
                    got_welcome = True
                    break
            if got_welcome:
                stats["registered"] += 1
            else:
                stats["no_welcome"] += 1
        # ここで sem 解放 → 接続は開いたまま hold
        await asyncio.sleep(hold)
    except asyncio.TimeoutError:
        stats["timeout"] += 1
    except OSError as e:
        key = f"oserr:{e.errno}"
        stats[key] = stats.get(key, 0) + 1
    finally:
        if writer is not None:
            writer.close()


def server_rss_kb(port):
    """ircserv の RSS(KB) を取得（見つからなければ None）。"""
    try:
        out = subprocess.check_output(["pgrep", "-f", "ircserv"]).split()
        pid = out[0].decode()
        rss = subprocess.check_output(["ps", "-o", "rss=", "-p", pid])
        return int(rss.strip())
    except Exception:
        return None


async def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=6667)
    ap.add_argument("--pass", dest="password", default="pass")
    ap.add_argument("--count", type=int, default=15000)
    ap.add_argument("--concurrency", type=int, default=500)
    ap.add_argument("--hold", type=float, default=3.0)
    args = ap.parse_args()

    stats = {"connected": 0, "registered": 0, "no_welcome": 0, "timeout": 0}
    sem = asyncio.Semaphore(args.concurrency)

    rss_before = server_rss_kb(args.port)
    t0 = time.monotonic()
    tasks = [
        asyncio.create_task(
            one_client(i, args.host, args.port, args.password, args.hold, stats, sem)
        )
        for i in range(args.count)
    ]
    # 接続が出そろった頃合いで RSS を測る
    await asyncio.sleep(min(args.hold, 2.0))
    rss_peak = server_rss_kb(args.port)
    await asyncio.gather(*tasks)
    elapsed = time.monotonic() - t0

    print("=" * 50)
    print(f"目標接続数     : {args.count}")
    print(f"TCP接続成功    : {stats['connected']}")
    print(f"登録成功(001)  : {stats['registered']}")
    print(f"001未受信      : {stats['no_welcome']}")
    print(f"タイムアウト   : {stats['timeout']}")
    for k, v in sorted(stats.items()):
        if k.startswith("oserr:"):
            print(f"{k:15}: {v}")
    print(f"所要時間       : {elapsed:.2f}s")
    if rss_before is not None and rss_peak is not None:
        print(f"サーバRSS      : {rss_before/1024:.1f}MB -> {rss_peak/1024:.1f}MB")
    print("=" * 50)

    ok = stats["registered"] >= args.count * 0.99
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))
