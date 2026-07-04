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
    if (!cli.isAuthenticated())
    {
        srv.sendToFd(cli.getFd(), Replies::ERR_NOTREGISTERED(cli.getNick()));
        return;
    }
    if (params.size() < 1)
    {
        srv.sendToFd(cli.getFd(), Replies::ERR_NEEDMOREPARAMS(cli.getNick(), "TOPIC"));
        return;
    }
    std::string chanName = params[0];
    std::map<std::string, Channel>& channels = srv.getChannels();
    if (channels.find(chanName) == channels.end())
    {
        srv.sendToFd(cli.getFd(), Replies::ERR_NOSUCHCHANNEL(cli.getNick(), chanName));
        return;
    }
    Channel& ch = channels[chanName];
    if (!(ch.isMember(cli.getFd())))
    {
        srv.sendToFd(cli.getFd(), Replies::ERR_NOTONCHANNEL(cli.getNick(), chanName));
        return;
    }

    // execute
    if (params.size() == 1)
    {
        std::string topic = ch.getTopic();
        if (topic.empty())
            srv.sendToFd(cli.getFd(), Replies::RPL_NOTOPIC(cli.getNick(), ch.getName()));
        else
            srv.sendToFd(cli.getFd(), Replies::RPL_TOPIC(cli.getNick(), ch.getName(), topic));
        return;
    }
    else
    {
        if (ch.isTopicRestricted() && !ch.isOperator(cli.getFd()))
        {
            srv.sendToFd(cli.getFd(), Replies::ERR_CHANOPRIVSNEEDED(cli.getNick(), ch.getName()));
            return;
        }
        std::string new_topic = params[1];
        ch.setTopic(new_topic);
        std::string msg = ":" + cli.getPrefix() + " TOPIC " + chanName + " :" + new_topic + "\r\n";
        srv.sendToChannel(ch, msg);
    }
}
