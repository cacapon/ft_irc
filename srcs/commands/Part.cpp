#include "Commands.hpp"

#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Server.hpp"

void Commands::handlePart(Server& srv, Client& client, std::vector<std::string>& params)
{
    // If it's not verified, ignore it
    if (!requireRegistered(srv, client))
        return;

    // If `params` is empty, return 461 ERR_NEEDMOREPARAMS.
    if (!requireParams(srv, client, params, 1, "PART"))
        return;

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
    Channel* ch = findChannelOrReply(srv, client, chanName);
    if (!ch)
        return;

    // Check if you're a member of that channel
    if (!requireMembership(srv, client, *ch, chanName))
        return;

    // Delete the client after sending a message
    broadcastWithOptionalReason(srv, *ch, client, "PART", chanName, reason);

    ch->removeMember(client.getFd());
    // If everyone leaves the channel, delete the channel.
    if (ch->getMembers().empty())
    {
        srv.getChannels().erase(chanName);
    }
}
