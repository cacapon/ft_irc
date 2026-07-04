#include "Commands.hpp"

#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

void Commands::handlePrivmsg(Server& srv, Client& client, std::vector<std::string>& params)
{
    // validate
    if (!client.isAuthenticated())
        return;
    if (params.empty())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NORECIPIENT(client.getNick(), "PRIVMSG"));
        return;
    }
    if (params.size() < 2 || params[1] == "")
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTEXTTOSEND(client.getNick()));
        return;
    }

    // check target
    std::string target = params[0];
    bool target_is_channel = (target[0] == '#' || target[0] == '&');
    if (target_is_channel)
    {
        // send to channel
        std::map<std::string, Channel>& channels = srv.getChannels();
        std::map<std::string, Channel>::iterator it = channels.find(target);

        if (it == channels.end())
        {
            srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
            return;
        }

        Channel& ch = it->second;
        if (!ch.isMember(client.getFd()))
        {
            srv.sendToFd(client.getFd(), Replies::ERR_CANNOTSENDTOCHAN(client.getNick(), target));
            return;
        }
        std::string msg = ":" + client.getPrefix() + " PRIVMSG " + target + " :" + params[1] + "\r\n";
        srv.sendToChannel(ch, msg, client.getFd());
        return;
    }
    else
    {
        // send to nick
        std::map<int, Client>& clients = srv.getClients();
        for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
        {
            if (it->second.getNick() == target)
            {
                std::string msg = ":" + client.getPrefix() + " PRIVMSG " + target + " :" + params[1] + "\r\n";
                srv.sendToFd(it->second.getFd(), msg);
                return;
            }
        }
    }
    srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
}
