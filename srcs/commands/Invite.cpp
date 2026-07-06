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
    if (!requireRegistered(srv, client))
        return;

    // Check if there are enough arguments
    if (!requireParams(srv, client, params, 2, "INVITE"))
        return;

    std::string target = params[0];
    std::string chanName = params[1];

    // Find a Channel
    Channel* ch = findChannelOrReply(srv, client, chanName);
    if (!ch)
        return;

    // Check if you're a member of that channel
    if (!requireMembership(srv, client, *ch, chanName))
        return;

    // Check whether you have operator privileges
    if (ch->isInviteOnly() && !requireChanOp(srv, client, *ch, chanName))
        return;

    // Check whether the target nick to be INVITED is connected to the server
    Client* targetClient = srv.findClientByNick(target);
    // An error occurs if there are no invitees
    if (!targetClient)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
        return;
    }
    int targetFd = targetClient->getFd();

    // Error if the other party is on the channel
    if (ch->isMember(targetFd))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_USERONCHANNEL(client.getNick(), target, chanName));
        return;
    }

    ch->addInvited(targetFd);
    // Notice
    srv.sendToFd(client.getFd(), Replies::RPL_INVITING(client.getNick(), chanName, target));
    std::string msg = ":" + client.getPrefix() + " INVITE " + target + " " + chanName + "\r\n";
    srv.sendToFd(targetFd, msg);
}
