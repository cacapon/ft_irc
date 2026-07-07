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
    if (!requireRegistered(srv, client))
        return;

    // Check if there are enough arguments
    if (!requireParams(srv, client, params, 2, "KICK"))
        return;

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
    Channel* ch = findChannelOrReply(srv, client, chanName);
    if (!ch)
        return;

    // Check if you're a member of that channel
    if (!requireMembership(srv, client, *ch, chanName))
        return;

    // Check whether the executor is an operator
    if (!requireChanOp(srv, client, *ch, chanName))
        return;

    // Check whether the nick to be KICKed is connected to the server
    Client* targetClient = srv.findClientByNick(target);
    // If it isn't found, an error occurs
    if (!targetClient)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
        return;
    }
    int targetFd = targetClient->getFd();

    // Check whether the target of the KICK command is a channel member you want to kick
    if (!ch->isMember(targetFd))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_USERNOTINCHANNEL(client.getNick(), target, chanName));
        return;
    }

    // Create a message and send it to everyone
    broadcastWithOptionalReason(srv, *ch, client, "KICK", chanName + " " + target, reason);

    // Remove the item from the channel
    ch->removeMember(targetFd);
    if (ch->getMembers().empty())
    {
        srv.getChannels().erase(chanName);
    }
}
