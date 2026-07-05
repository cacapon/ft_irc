#include "Commands.hpp"

#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

void Commands::handleKick(Server& srv, Client& client, std::vector<std::string>& params)
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
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(client.getNick(), "KICK"));
        return;
    }

    std::string chanName = params[0];
    std::string target = params[1];
    std::string reason;
    // If there are parameters, include an explanation as well
    if (params.size() > 2)
    {
        reason = params[2];
    }
    else
    {
        reason = "";
    }

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

    // Check whether the executor is an operator
    if (!ch.isOperator(client.getFd()))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_CHANOPRIVSNEEDED(client.getNick(), chanName));
        return;
    }

    // Check whether the fd to be KICKed is connected to the server
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
    // If it isn't found, an error occurs
    if (targetFd == -1)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
        return;
    }

    // Check whether the target of the KICK command is a channel member you want to kick
    if (!ch.isMember(targetFd))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_USERNOTINCHANNEL(client.getNick(), target, chanName));
        return;
    }

    // Create a message and send it to everyone
    std::string msg = ":" + client.getPrefix() + " KICK " + chanName + " " + target;
    if (!reason.empty())
    {
        msg += " :" + reason;
    }
    msg += "\r\n";
    srv.sendToChannel(ch, msg);

    // Remove the item from the channel
    ch.removeMember(targetFd);
    if (ch.getMembers().empty())
    {
        channels.erase(chanName);
    }
}
