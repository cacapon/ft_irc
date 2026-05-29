#include <iostream>
#include <cstdio>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <map>
#include <sstream>

static void usage() {
    std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
}

int main(int argc, char **argv) {
    if (argc != 3)
        return (usage(), 1);

    int port = atoi(argv[1]);
    std::string password = argv[2];

    // ① ソケット作成
    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) { perror("socket"); return 1; }

    // ② アドレス再利用
    int opt = 1; // 1 = 有効、0 = 無効
    // なるほど、何でも型を受け取れる＝渡す型だけだとサイズがわからないので、sizeofで型のサイズを渡している。
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // ③ ノンブロッキック化
    // fdに フラグをセット ノンブロッキングで
    fcntl(serverFd, F_SETFL, O_NONBLOCK);
    
    // ④ bind
    // htons = Host TO Network Short htons() で変換しないと相手に間違ったポート番号が伝わる
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family         = AF_INET;      //  Socket INternet family
    addr.sin_addr.s_addr    = INADDR_ANY;   //  Socket INternet address = IN ADDRess ANY
    addr.sin_port           = htons(port);  //  Socket INternet port 
    if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }

    // ⑤ listen
    if (listen(serverFd, SOMAXCONN) < 0) {
        perror("listen"); return 1;
    }
    std::cout << "Server listening on port " << port << std::endl;
    // ⑥ poll ループ（今は新接続のログだけ）
    std::vector<struct pollfd> pollfds;
    std::map<int, std::string> recvBufs;
    struct pollfd pfd;
    pfd.fd      = serverFd; // 監視するfd
    pfd.events  = POLLIN;   // 監視するイベント（何を待つか）
    pfd.revents = 0;        // 実際に起きたイベント（OSが書き込む） 0なにもおきてないしょきか
    pollfds.push_back(pfd);

    while(true){
        //poll(配列の先頭ポインタ, 監視するfdの数, タイムアウト（-1 = 永遠に待つ）)
        int ret = poll(&pollfds[0], pollfds.size(), -1);
        if (ret < 0) {
            perror("poll"); break;
        }

        // revents で何が起きたか確認する
        for (size_t i = 0; i < pollfds.size(); i++) {
            // ビット演算でPOLLINが含まれているかチェック
            if (pollfds[i].revents & POLLIN) {
                if (pollfds[i].fd == serverFd) {
                    int clientFd = accept(serverFd, NULL, NULL);
                    if (clientFd >=0) {
                        std::cout << "New client connected: fd=" << clientFd << std::endl;

                        struct pollfd clientPfd;
                        clientPfd.fd        = clientFd;
                        clientPfd.events    = POLLIN;
                        clientPfd.revents   = 0;
                        pollfds.push_back(clientPfd);
                        recvBufs[clientFd] = "";
                    }
                }
                else {
                    char buf[512]; // TODO　ヘッダーか何かで定数を管理したほうがいいかも？
                    // recv(受信fd, 受信データ格納バッファ, 受信最大バイト数, フラグ(0))
                    int bytes = recv(pollfds[i].fd, buf, sizeof(buf) - 1, 0);
                    if (bytes <= 0) {
                        // disconnect
                        std::cout << "Client disconnected: fd=" << pollfds[i].fd << std::endl;
                        close(pollfds[i].fd);
                        recvBufs.erase(pollfds[i].fd);
                        pollfds.erase(pollfds.begin() + i);
                        i--;
                    }
                    else {
                        int fd = pollfds[i].fd;
                        recvBufs[fd] += std::string(buf, bytes); // バッファ追記

                        // \r\nが見つかるまで1行ずつ取り出す
                        size_t pos;
                        while ((pos = recvBufs[fd].find("\r\n")) != std::string::npos) {
                            std::string line = recvBufs[fd].substr(0, pos);
                            recvBufs[fd].erase(0, pos + 2);
                            //std::cout << "Line: " << line << std::endl;
                            std::istringstream ss(line);
                            std::string cmd;
                            ss >> cmd;
                            std::cout << "Command: " << cmd << std::endl;
                            if (cmd == "PASS") {
                                std::string pass;
                                ss >> pass;
                                if (pass == password)
                                    std::cout << "PASS OK" << std::endl;
                                else
                                    std::cout << "PASS NG" << std::endl;
                            }
                            else if (cmd == "NICK") {
                                std::string nick;
                                ss >> nick;
                                std::cout << "NICK: " << nick << std::endl;
                            }
                            else if (cmd == "USER") {
                                std::string user;
                                ss >> user;
                                std::cout << "USER: " << user << std::endl;
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}