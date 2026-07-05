#include "Commands.hpp"

#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

bool Commands::isValidNick(const std::string& nick)
{
    if (nick.empty() || nick.size() > 9)
        return false;

    const std::string special = "[]\\`_^{|}";

    if (!std::isalpha(static_cast<unsigned char>(nick[0])) && special.find(nick[0]) == std::string::npos)
        return false;

    for (size_t i = 1; i < nick.size(); ++i)
    {
        char c = nick[i];
        if (!std::isalnum(static_cast<unsigned char>(c)) && special.find(c) == std::string::npos && c != '-')
            return false;
    }

    return true;
}

void Commands::handleNick(Server& srv, Client& client, std::vector<std::string>& params)
{
    std::string target = client.getNick().empty() ? "*" : client.getNick();

    // NICK is rejected until the password has been accepted.
    if (!client.isPassOk())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTREGISTERED(target));
        return;
    }

    if (params.empty())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NONICKNAMEGIVEN(target));
        return;
    }
    std::string nick = params[0];

    if (!isValidNick(nick))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_ERRONEUSNICKNAME(target, nick));
        return;
    }

    // Duplicate Check
    std::map<int, Client>& clients = srv.getClients();
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (it->first != client.getFd() && it->second.getNick() == nick)
        {
            srv.sendToFd(client.getFd(), Replies::ERR_NICKNAMEINUSE(target, nick));
            return;
        }
    }

    // A no-op nick change (same nick) needs no state update or broadcast.
    if (nick == client.getNick())
        return;

    // When an already-registered client changes its nick, echo the change to
    // itself and to everyone sharing a channel so their views stay consistent.
    // Built with the old prefix before setNick; done only post-registration so
    // the initial nick assignment during registration is not broadcast.
    if (client.isWelcomed())
    {
        std::string notify = ":" + client.getPrefix() + " NICK :" + nick + "\r\n";
        srv.sendToFd(client.getFd(), notify);
        std::map<std::string, Channel>& channels = srv.getChannels();
        for (std::map<std::string, Channel>::iterator it = channels.begin(); it != channels.end(); ++it)
        {
            if (it->second.isMember(client.getFd()))
                srv.sendToChannel(it->second, notify, client.getFd());
        }
    }
    client.setNick(nick);
    tryRegister(srv, client);
}
