#include <cstdlib>
#include <iostream>
#include <string>

#include "Server.hpp"

static void usage()
{
    std::cout << "Usage: ./ircserv <port 6665-6669> <password>" << std::endl;
}

int main(int argc, char** argv)
{
    if (argc != 3)
        return (usage(), 1);

    char* end;
    long port = std::strtol(argv[1], &end, 10);
    if (*end != '\0' || port < 6665 || port > 6669)
        return (usage(), 1);
    std::string password = argv[2];
    Server server(static_cast<int>(port), password);
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
