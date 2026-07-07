#include "Commands.hpp"

#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

/**
 * @brief
 *
 * @param srv
 * @param cli
 * @param params
 * @note ERR_NEEDMOREPARAMS, ERR_NOTONCHANNEL, RPL_NOTOPIC, RPL_TOPIC, ERR_CHANOPRIVSNEEDED, ERR_NOCHANMODES
 */
void Commands::handleTopic(Server& srv, Client& cli, std::vector<std::string>& params)
{
    // validate
    if (!requireRegistered(srv, cli))
        return;
    if (!requireParams(srv, cli, params, 1, "TOPIC"))
        return;
    std::string chanName = params[0];
    Channel* ch = findChannelOrReply(srv, cli, chanName);
    if (!ch)
        return;
    if (!requireMembership(srv, cli, *ch, chanName))
        return;

    // execute
    if (params.size() == 1)
    {
        std::string topic = ch->getTopic();
        if (topic.empty())
            srv.sendToFd(cli.getFd(), Replies::RPL_NOTOPIC(cli.getNick(), ch->getName()));
        else
            srv.sendToFd(cli.getFd(), Replies::RPL_TOPIC(cli.getNick(), ch->getName(), topic));
        return;
    }
    else
    {
        if (ch->isTopicRestricted() && !requireChanOp(srv, cli, *ch, ch->getName()))
            return;
        std::string new_topic = params[1];
        ch->setTopic(new_topic);
        std::string msg = ":" + cli.getPrefix() + " TOPIC " + chanName + " :" + new_topic + "\r\n";
        srv.sendToChannel(*ch, msg);
    }
}
