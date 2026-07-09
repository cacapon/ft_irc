#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

#include "Server.hpp"

static void usage()
{
    std::cout << "Usage: ./ircserv <port 6665-6669> <password>" << std::endl;
}

static bool validate_port(const char* str)
{
    if (*str == '\0')
        return false;

    size_t len = std::strlen(str);
    if (len > 5)  // 5桁以上は絶対に範囲外なので即弾く
        return false;

    for (size_t i = 0; str[i]; i++)
    {
        if (!std::isdigit(static_cast<unsigned char>(str[i])))
            return false;  // 数字以外が混ざってたらNG(符号なしでいい前提)
    }

    long val = std::strtol(str, NULL, 10);  // ここではオーバーフローしない

    return (val >= 6665 && val <= 6669);
}

int main(int argc, char** argv)
{
    if (argc != 3)
        return (usage(), 1);

    if (!validate_port(argv[1]))
        return (usage(), 1);
    int port = atoi(argv[1]);
    std::string password = argv[2];
    Server server(port, password);
    try
    {
        server.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    return 0;
}
