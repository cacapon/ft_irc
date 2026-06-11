#include "Commands.hpp"

#include <sys/socket.h>

// private
Commands::Commands()
{
}
Commands::Commands(const Commands& src)
{
    (void)src;
}
Commands& Commands::operator=(const Commands& other)
{
    (void)other;
    return *this;
}
Commands::~Commands()
{
}

void Commands::handlePass(Client& client, std::istringstream& ss, const std::string& password)
{
    std::string pass;
    ss >> pass;
    client.setPassOk((pass == password));
}
void Commands::handleNick(Client& client, std::istringstream& ss)
{
    std::string nick;
    ss >> nick;
    client.setNick(nick);
}
void Commands::handleUser(Client& client, std::istringstream& ss)
{
    std::string user;
    ss >> user;
    client.setUser(user);

    if (client.isAuthenticated())
    {
        std::string reply = ":server 001 " + client.getNick() + " :Welcome!\r\n";
        send(client.getFd(), reply.c_str(), reply.size(), 0);
    }
}

// public
void Commands::dispatch(Client& client, const std::string& line, const std::string& password)
{
    std::istringstream ss(line);
    std::string cmd;
    ss >> cmd;
    if (cmd == "PASS")
        handlePass(client, ss, password);
    else if (cmd == "NICK")
        handleNick(client, ss);
    else if (cmd == "USER")
        handleUser(client, ss);
}