#include <iostream>

static void usage() {
    std::cout << "Usage: ./ircserv <port> <password>" << std::endl;
}

int main(int argc, char **argv) {
    (void) argv;

    if (argc != 3)
        return (usage(), 1);

    std::cout << "hello" << std::endl;
    return 0;
}