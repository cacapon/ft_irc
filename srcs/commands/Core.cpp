#include "Commands.hpp"

#include <cctype>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

// PASS/NICK/USER may arrive in any order, so call this from the end of each
// handler to send 001-004 once they are all set. The _welcomed flag prevents
// double sending.
void Commands::tryRegister(Server& srv, Client& client)
{
    if (client.isWelcomed() || !client.isAuthenticated())
        return;

    std::string nick = client.getNick();
    std::string user = client.getUser();

    srv.sendToFd(client.getFd(), Replies::RPL_WELCOME(nick, user, client.getHost()));
    srv.sendToFd(client.getFd(), Replies::RPL_YOURHOST(nick));
    srv.sendToFd(client.getFd(), Replies::RPL_CREATED(nick, __DATE__));
    srv.sendToFd(client.getFd(), Replies::RPL_MYINFO(nick));
    client.setWelcomed(true);
}

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

/**
 * @brief In accordance with the IRC2810-2.3.1 specifications,
the command is parsed into prefix, command, and params.
 *
 * @param line
 * @return Message
 */
Message Commands::parseLine(const std::string& line)
{
    Message msg;
    size_t pos = 0;

    // get prefix
    if (!line.empty() && line[0] == ':')
    {
        size_t end = line.find(' ');
        if (end == std::string::npos)
        {
            msg.prefix = line.substr(1);
            pos = line.size();
        }
        else
        {
            msg.prefix = line.substr(1, end - 1);
            pos = end + 1;
        }
    }

    // get command
    while (pos < line.size() && line[pos] == ' ')
        pos++;
    size_t start = pos;
    while (pos < line.size() && line[pos] != ' ')
        pos++;
    msg.command = line.substr(start, pos - start);

    for (std::string::iterator it = msg.command.begin(); it != msg.command.end(); ++it)
        *it = std::toupper(static_cast<unsigned char>(*it));
    // get params
    while (pos < line.size())
    {
        while (pos < line.size() && line[pos] == ' ')
            pos++;
        if (pos >= line.size())
            break;

        if (line[pos] == ':')
        {
            msg.params.push_back(line.substr(pos + 1));
            break;
        }
        start = pos;
        while (pos < line.size() && line[pos] != ' ')
            pos++;
        msg.params.push_back(line.substr(start, pos - start));
    }
    return msg;
}

// public
void Commands::dispatch(Server& srv, Client& client, const std::string& line)
{
    typedef void (*HandlerFunc)(Server&, Client&, std::vector<std::string>&);
    static std::map<std::string, HandlerFunc> table;
    if (table.empty())
    {
        table["PING"] = &Commands::handlePing;
        table["PASS"] = &Commands::handlePass;
        table["NICK"] = &Commands::handleNick;
        table["USER"] = &Commands::handleUser;
        table["JOIN"] = &Commands::handleJoin;
        table["PRIVMSG"] = &Commands::handlePrivmsg;
        table["PART"] = &Commands::handlePart;
        table["MODE"] = &Commands::handleMode;
        table["KICK"] = &Commands::handleKick;
        table["INVITE"] = &Commands::handleInvite;
        table["QUIT"] = &Commands::handleQuit;
        table["TOPIC"] = &Commands::handleTopic;
    }
    Message msg = parseLine(line);
    if (msg.command.empty())
        return;
    std::map<std::string, HandlerFunc>::const_iterator it = table.find(msg.command);
    if (it != table.end())
        it->second(srv, client, msg.params);
    else
        srv.sendToFd(client.getFd(), Replies::ERR_UNKNOWNCOMMAND(client.getNick(), msg.command));
}
