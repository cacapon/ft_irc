# 耐久性・負荷 E2E テスト

`ircserv` の耐久性を確認するためのテスト群。

- **ばか長文字列** → `long_string.sh`（nc ベース）
- **大人数同時接続（万単位）** → `flood_connect.py`（asyncio ベース）
- **機能確認の基準クライアント** → `irssi`（`brew install irssi`、手動）

nc は 1 接続 = 1 プロセスで数万接続に耐えられないため、大人数テストは単一プロセスで
多数のソケットを張れる `flood_connect.py` を使う。nc は機能・長文字列テスト用。

## 実行手順

```sh
make
./ircserv 6667 pass &          # 別ターミナル推奨

# ばか長文字列
./tests/stress/long_string.sh 127.0.0.1 6667 pass

# 大人数同時接続（10,000本を確立し hold 80秒保持）
python3 tests/stress/flood_connect.py --port 6667 --pass pass \
    --count 10000 --concurrency 120 --hold 80
```

`flood_connect.py` の `--concurrency` は「接続確立フェーズ」だけを律速する
（listen backlog あふれ防止）。hold 中は解放されるので接続は `--count` 本まで積み上がる。

## 測定結果（2026-07-01 / Apple Silicon macOS）

### 大人数同時接続

| 目標 | TCP成功 | 001登録成功 | 失敗 | 最大同時接続 | サーバRSS |
|---|---|---|---|---|---|
| 10,000 | 10,000 | 10,000 | 0 | 10,000 | 3.6MB |

- **10,000 同時接続をクラッシュなしで処理**。Client 1個あたり約200Bで極めて省メモリ。
- 到達速度は accept が律速（後述）。

### ばか長文字列（1クライアントが1行を送りつける）

| 入力 | 処理時間 | サーバRSS |
|---|---|---|
| 1MB | 0.08s | 7MB |
| 4MB | 0.68s | 238MB |
| 8MB | 2.69s | 494MB |

- クラッシュはしないが、**処理時間・メモリが入力に対して二乗で増加**（8MB入力でRSS約500MB）。
- 64MB を流すと 2.5GB / 数分ストール（1クライアントで容易にDoS可能）。

## 判明した弱点と対策（サーバ側）

1. **`Client::getRecvBuf()` が値返し**（`srcs/Client.cpp`）
   `Server::receiveData` のループ（`srcs/Server.cpp`）で毎回バッファ全体をコピー＋走査 →
   長い1行で O(n²)。**戻り値を `const std::string&` に変更**するのが最優先。

2. **行長の上限が無い**（`srcs/Server.cpp receiveData`）
   `\r\n` が来るまで recv バッファが無制限に伸びる。**IRC標準の512バイト上限を実装**し、
   超過分は破棄 or エラー（`ERR_INPUTTOOLONG` 相当）。

3. **client fd がブロッキング**（非ブロッキング化は `_serverFd` のみ）
   遅い読み手への `send` が単一スレッド poll ループ全体を止める。
   **client fd も O_NONBLOCK にし、送信バッファ（per-client 送信キュー＋POLLOUT）を実装**。

4. **accept が poll 1周につき1件 / backlog=SOMAXCONN(128)**（`srcs/Server.cpp pollLoop`）
   万単位への到達速度が律速される。**1周で drain するまで accept をループ**すると改善。

## さらに大規模（1.6万超の同時接続）にするには

macOS の ephemeral ポートは 49152–65535 の 16,384 個。単一IP→単一ポートは
4タプルの一意性でこの数が同時接続の上限。超えるには（要 sudo）:

```sh
# ephemeral 範囲を拡大
sudo sysctl -w net.inet.ip.portrange.first=10000
# listen backlog 拡大
sudo sysctl -w kern.ipc.somaxconn=4096
# もしくは loopback エイリアスで宛先IPを増やす（4タプルを分散）
sudo ifconfig lo0 alias 127.0.0.2 up
```
