#include "Commands.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

// Shared precondition guards for command handlers: registration, parameter
// count, and channel membership/operator checks that were duplicated across
// commands/*.cpp.

std::string Commands::replyTarget(const Client& client)
{
    return client.getNick().empty() ? "*" : client.getNick();
}

bool Commands::requireRegistered(Server& srv, Client& client)
{
    if (!client.isAuthenticated())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTREGISTERED(replyTarget(client)));
        return false;
    }
    return true;
}

bool Commands::requireParams(Server& srv, Client& client, const std::vector<std::string>& params, size_t minParams,
                              const std::string& command)
{
    if (params.size() < minParams)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(replyTarget(client), command));
        return false;
    }
    return true;
}

bool Commands::requireMembership(Server& srv, Client& client, Channel& ch, const std::string& chanName)
{
    if (!ch.isMember(client.getFd()))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTONCHANNEL(replyTarget(client), chanName));
        return false;
    }
    return true;
}

bool Commands::requireChanOp(Server& srv, Client& client, Channel& ch, const std::string& chanName)
{
    if (!ch.isOperator(client.getFd()))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_CHANOPRIVSNEEDED(replyTarget(client), chanName));
        return false;
    }
    return true;
}

Channel* Commands::findChannelOrReply(Server& srv, Client& client, const std::string& chanName)
{
    Channel* ch = srv.findChannel(chanName);
    if (!ch)
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(replyTarget(client), chanName));
    return ch;
}

void Commands::broadcastWithOptionalReason(Server& srv, Channel& ch, Client& client, const std::string& command,
                                            const std::string& targetPart, const std::string& reason)
{
    std::string msg = ":" + client.getPrefix() + " " + command + " " + targetPart;
    if (!reason.empty())
        msg += " :" + reason;
    msg += "\r\n";
    srv.sendToChannel(ch, msg);
}
