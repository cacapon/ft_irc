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
    if (!requireRegistered(srv, client))
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
        // Note: an unknown channel replies ERR_NOSUCHNICK (not ERR_NOSUCHCHANNEL)
        // here, so findChannelOrReply is not used to keep that reply code intact.
        Channel* ch = srv.findChannel(target);
        if (!ch)
        {
            srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
            return;
        }

        if (!ch->isMember(client.getFd()))
        {
            srv.sendToFd(client.getFd(), Replies::ERR_CANNOTSENDTOCHAN(client.getNick(), target));
            return;
        }
        std::string msg = ":" + client.getPrefix() + " PRIVMSG " + target + " :" + params[1] + "\r\n";
        srv.sendToChannel(*ch, msg, client.getFd());
        return;
    }
    else
    {
        // send to nick
        Client* targetClient = srv.findClientByNick(target);
        if (targetClient)
        {
            std::string msg = ":" + client.getPrefix() + " PRIVMSG " + target + " :" + params[1] + "\r\n";
            srv.sendToFd(targetClient->getFd(), msg);
            return;
        }
    }
    srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
}
