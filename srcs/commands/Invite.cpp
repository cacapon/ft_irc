#include "Commands.hpp"

#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

void Commands::handleInvite(Server& srv, Client& client, std::vector<std::string>& params)
{
    // If it's not verified, ignore it
    if (!client.isAuthenticated())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTREGISTERED(client.getNick()));
        return;
    }

    // Check if there are enough arguments
    if (params.size() < 2)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(client.getNick(), "INVITE"));
        return;
    }

    std::string target = params[0];
    std::string chanName = params[1];

    // Find a Channel
    std::map<std::string, Channel>& channels = srv.getChannels();
    if (channels.find(chanName) == channels.end())
    {
        // If not, return a 403 error.
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
        return;
    }

    Channel& ch = channels[chanName];

    // Check if you're a member of that channel
    if (!(ch.isMember(client.getFd())))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTONCHANNEL(client.getNick(), chanName));
        return;
    }

    // Check whether you have operator privileges
    if (ch.isInviteOnly() && !(ch.isOperator(client.getFd())))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_CHANOPRIVSNEEDED(client.getNick(), chanName));
        return;
    }

    // Check whether the target fd to be INVITED is connected to the server
    int targetFd = -1;
    std::map<int, Client>& clients = srv.getClients();
    for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
    {
        if (it->second.getNick() == target)
        {
            targetFd = it->second.getFd();
            break;
        }
    }

    // An error occurs if there are no invitees
    if (targetFd == -1)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
        return;
    }

    // Error if the other party is on the channel
    if (ch.isMember(targetFd))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_USERONCHANNEL(client.getNick(), target, chanName));
        return;
    }

    ch.addInvited(targetFd);
    // Notice
    srv.sendToFd(client.getFd(), Replies::RPL_INVITING(client.getNick(), chanName, target));
    std::string msg = ":" + client.getPrefix() + " INVITE " + target + " " + chanName + "\r\n";
    srv.sendToFd(targetFd, msg);
}
