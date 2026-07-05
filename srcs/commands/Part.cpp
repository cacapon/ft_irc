#include "Commands.hpp"

#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

void Commands::handlePart(Server& srv, Client& client, std::vector<std::string>& params)
{
    // If it's not verified, ignore it
    if (!client.isAuthenticated())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTREGISTERED(client.getNick()));
        return;
    }

    // If `params` is empty, return 461 ERR_NEEDMOREPARAMS.
    if (params.empty())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(client.getNick(), "PART"));
        return;
    }

    std::string chanName = params[0];
    std::string reason;
    // If there are parameters, include an explanation as well.
    if (params.size() > 1)
    {
        reason = params[1];
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

    // Delete the client after sending a message
    std::string msg = ":" + client.getPrefix() + " PART " + chanName;
    if (!reason.empty())
    {
        msg += " :" + reason;
    }
    msg += "\r\n";
    srv.sendToChannel(ch, msg);

    ch.removeMember(client.getFd());
    // If everyone leaves the channel, delete the channel.
    if (ch.getMembers().empty())
    {
        channels.erase(chanName);
    }
}
