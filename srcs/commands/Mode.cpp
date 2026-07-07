#include "Commands.hpp"

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

// helper
void Commands::recordAppliedMode(std::string& appliedModes, char& lastSign, char modeChar, bool adding)
{
    if (lastSign != (adding ? '+' : '-'))
    {
        appliedModes += (adding ? '+' : '-');
        lastSign = adding ? '+' : '-';
    }
    appliedModes += modeChar;
}

void Commands::handleMode(Server& srv, Client& cli, std::vector<std::string>& params)
{
    // varidate
    if (!requireRegistered(srv, cli))
        return;
    if (!requireParams(srv, cli, params, 1, "MODE"))
        return;
    std::string chanName = params[0];
    Channel* chPtr = findChannelOrReply(srv, cli, chanName);
    if (!chPtr)
        return;
    Channel& ch = *chPtr;
    if (params.size() == 1)
    {
        srv.sendToFd(cli.getFd(),
                     Replies::RPL_CHANNELMODEIS(cli.getNick(), ch.getName(), ch.getModeString(), ch.getModeParams()));
        return;
    }

    if (!requireChanOp(srv, cli, ch, ch.getName()))
        return;

    std::string modeString = params[1];
    size_t argldx = 2;
    bool adding = true;
    std::string appliedModes = "";
    std::string appliedArgs = "";
    char lastSign = '\0';
    for (size_t i = 0; i < modeString.size(); i++)
    {
        char c = modeString[i];
        if (c == '+')
        {
            adding = true;
            continue;
        }
        if (c == '-')
        {
            adding = false;
            continue;
        }

        switch (c)
        {
            case 'i':
                ch.setInviteOnly(adding);
                recordAppliedMode(appliedModes, lastSign, c, adding);
                break;
            case 't':
                ch.setTopicRestricted(adding);
                recordAppliedMode(appliedModes, lastSign, c, adding);
                break;
            case 'k':
                if (!adding)
                {
                    ch.setKey("");
                    recordAppliedMode(appliedModes, lastSign, c, adding);
                    break;
                }
                if (!requireParams(srv, cli, params, argldx + 1, "MODE"))
                    break;
                ch.setKey(params[argldx++]);
                recordAppliedMode(appliedModes, lastSign, c, adding);
                appliedArgs += " " + params[argldx - 1];
                break;
            case 'l':
            {
                if (!adding)
                {
                    ch.setLimit(0);
                    recordAppliedMode(appliedModes, lastSign, c, adding);
                    break;
                }
                if (!requireParams(srv, cli, params, argldx + 1, "MODE"))
                    break;
                const std::string& limitStr = params[argldx++];
                bool valid = !limitStr.empty();
                for (size_t j = 0; j < limitStr.size() && valid; ++j)
                {
                    if (!std::isdigit(static_cast<unsigned char>(limitStr[j])))
                        valid = false;
                }
                if (!valid)
                {
                    srv.sendToFd(cli.getFd(), Replies::ERR_UNKNOWNMODE(cli.getNick(), c, ch.getName()));
                    break;
                }
                int limit = atoi(limitStr.c_str());
                ch.setLimit(limit);
                recordAppliedMode(appliedModes, lastSign, c, adding);
                appliedArgs += " " + limitStr;
                break;
            }
            case 'o':
            {
                if (!requireParams(srv, cli, params, argldx + 1, "MODE"))
                    break;
                std::string target_nick = params[argldx++];
                Client* target_cli = srv.findClientByNick(target_nick);
                if (!target_cli)
                {
                    srv.sendToFd(cli.getFd(), Replies::ERR_NOSUCHNICK(cli.getNick(), target_nick));
                    break;
                }
                if (!ch.isMember(target_cli->getFd()))
                {
                    srv.sendToFd(cli.getFd(), Replies::ERR_USERNOTINCHANNEL(cli.getNick(), target_nick, ch.getName()));
                    break;
                }
                if (adding)
                    ch.addOperator(target_cli->getFd());
                else
                    ch.removeOperator(target_cli->getFd());
                recordAppliedMode(appliedModes, lastSign, c, adding);
                appliedArgs += " " + params[argldx - 1];
                break;
            }
            default:
                srv.sendToFd(cli.getFd(), Replies::ERR_UNKNOWNMODE(cli.getNick(), c, ch.getName()));
                break;
        }
    }
    //  broadcast
    //  :nick!user@host MODE #chan +it
    if (!appliedModes.empty())
    {
        std::string modeMsg = ":" + cli.getPrefix() + " MODE " + chanName + " " + appliedModes + appliedArgs + "\r\n";
        srv.sendToChannel(ch, modeMsg);
    }
}
