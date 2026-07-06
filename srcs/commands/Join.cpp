#include "Commands.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

void Commands::handleJoin(Server& srv, Client& client, std::vector<std::string>& params)
{
    // If it's not verified, ignore it
    if (!requireRegistered(srv, client))
        return;
    // If `chanName` is missing, send 461 (ERR_NEEDMOREPARAMS) to the client and return.
    if (!requireParams(srv, client, params, 1, "JOIN"))
        return;

    // Read one channel name
    std::string chanName = params[0];

    // Name Validation
    // The first character is not # or &,
    // OR the channel name contains no characters (ERR_NOSUCHCHANNEL)
    if (chanName.size() < 2 || (chanName[0] != '#' && chanName[0] != '&'))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
        return;
    }
    // Are the characters used in the channel name correct?
    for (size_t i = 0; i < chanName.size(); ++i)
    {
        char c = chanName[i];
        if (c == ' ' || c == ',' || c == '\a')
        {
            srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
            return;
        }
    }

    // Find/Create a Channel
    std::map<std::string, Channel>& channels = srv.getChannels();
    if (channels.find(chanName) == channels.end())
    {
        // If none exists, create a new one
        Channel& ch = channels.insert(std::make_pair(chanName, Channel(chanName))).first->second;
        ch.addMember(client.getFd());
        ch.addOperator(client.getFd());
    }
    else
    {
        Channel& ch = channels[chanName];
        // If you're already a member, return
        if (ch.isMember(client.getFd()))
            return;
        // i-mode Error Handling
        if (ch.isInviteOnly() && !ch.isInvited(client.getFd()))
        {
            srv.sendToFd(client.getFd(), Replies::ERR_INVITEONLYCHAN(client.getNick(), chanName));
            return;
        }
        // k-mode Error Handling
        if (!ch.getKey().empty() && (params.size() < 2 || params[1] != ch.getKey()))
        {
            srv.sendToFd(client.getFd(), Replies::ERR_BADCHANNELKEY(client.getNick(), chanName));
            return;
        }
        // l-mode Error Handling
        if (ch.getLimit() > 0 && (static_cast<int>(ch.getMembers().size()) >= ch.getLimit()))
        {
            srv.sendToFd(client.getFd(), Replies::ERR_CHANNELISFULL(client.getNick(), chanName));
            return;
        }
        ch.addMember(client.getFd());
        ch.removeInvited(client.getFd());
    }
    // Here, we retrieve `ch` again (it definitely exists by now).
    Channel& ch = channels[chanName];
    // Message when participation is successful
    std::string joinMsg = ":" + client.getPrefix() + " JOIN " + chanName + "\r\n";
    srv.sendToChannel(ch, joinMsg);

    // topic
    if (!ch.getTopic().empty())
    {
        srv.sendToFd(client.getFd(), Replies::RPL_TOPIC(client.getNick(), chanName, ch.getTopic()));
    }
    else
    {
        srv.sendToFd(client.getFd(), Replies::RPL_NOTOPIC(client.getNick(), chanName));
    }

    // RFC 2812 requires 353/366 right after JOIN so the joining client can build its
    // nicklist; reference clients (e.g. irssi) rely on this instead of the JOIN message alone.
    const std::set<int>& members = ch.getMembers();
    std::map<int, Client>& clients = srv.getClients();
    std::string names;
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        std::map<int, Client>::iterator clientIt = clients.find(*it);
        if (clientIt == clients.end())
            continue;
        if (!names.empty())
            names += " ";
        if (ch.isOperator(*it))
            names += "@";
        names += clientIt->second.getNick();
    }
    srv.sendToFd(client.getFd(), Replies::RPL_NAMREPLY(client.getNick(), chanName, names));
    srv.sendToFd(client.getFd(), Replies::RPL_ENDOFNAMES(client.getNick(), chanName));
    return;
}
