#include "Commands.hpp"

#include <sys/socket.h>

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

// private
Commands::Commands()
{
}
Commands::Commands(const Commands& src)
{
    (void)src;
}
Commands& Commands::operator=(const Commands& other)
{
    (void)other;
    return *this;
}
Commands::~Commands()
{
}

/**
 * @brief In accordance with the IRC2810-2.3.1 specifications,
the command is parsed into prefix, command, and params.
 *
 * @param line
 * @return Message
 */
Message Commands::parseLine(const std::string& line)
{
    Message msg;
    size_t pos = 0;

    // get prefix
    if (!line.empty() && line[0] == ':')
    {
        size_t end = line.find(' ');
        if (end == std::string::npos)
        {
            msg.prefix = line.substr(1);
            pos = line.size();
        }
        else
        {
            msg.prefix = line.substr(1, end - 1);
            pos = end + 1;
        }
    }

    // get command
    while (pos < line.size() && line[pos] == ' ')
        pos++;
    size_t start = pos;
    while (pos < line.size() && line[pos] != ' ')
        pos++;
    msg.command = line.substr(start, pos - start);

    // get params
    while (pos < line.size())
    {
        while (pos < line.size() && line[pos] == ' ')
            pos++;
        if (pos >= line.size())
            break;

        if (line[pos] == ':')
        {
            msg.params.push_back(line.substr(pos + 1));
            break;
        }
        start = pos;
        while (pos < line.size() && line[pos] != ' ')
            pos++;
        msg.params.push_back(line.substr(start, pos - start));
    }
    return msg;
}

void Commands::handlePing(Server& srv, Client& cli, std::vector<std::string>& params)
{
    std::string token = params.empty() ? "" : params[0];
    std::string msg = ":" SERVER_NAME " PONG " SERVER_NAME " :" + token + "\r\n";
    srv.sendToFd(cli.getFd(), msg);
}

void Commands::handlePass(Server& srv, Client& client, std::vector<std::string>& params)
{
    if (params.empty())
    {
        return;
    }
    std::string pass = params[0];
    client.setPassOk((pass == srv.getPassword()));
}
void Commands::handleNick(Server&, Client& client, std::vector<std::string>& params)
{
    if (params.empty())
    {
        return;
    }
    std::string nick = params[0];
    client.setNick(nick);
}
void Commands::handleUser(Server&, Client& client, std::vector<std::string>& params)
{
    if (params.empty())
    {
        return;
    }
    std::string user = params[0];
    client.setUser(user);

    if (client.isAuthenticated())
    {
        std::string reply = ":server 001 " + client.getNick() + " :Welcome!\r\n";
        send(client.getFd(), reply.c_str(), reply.size(), 0);
    }
}

void Commands::handleJoin(Server& srv, Client& client, std::vector<std::string>& params)
{
    // 未認証なら無視
    if (!client.isAuthenticated())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTREGISTERED(client.getNick()));
        return;
    }
    // chanNameがなければ461(ERR_NEEDMOREPARAMS)をclientに送ってreturn
    if (params.empty())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(client.getNick(), "JOIN"));
        return;
    }

    // チャンネル名を1つ読む
    std::string chanName = params[0];

    // 名前検証
    // 先頭が#, & 以外　OR　チャンネル名が1文字もない　(ERR_NOSUCHCHANNEL)
    if (chanName.size() < 2 || (chanName[0] != '#' && chanName[0] != '&'))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
        return;
    }
    // チャンネル名に使われている文字が正しいか
    for (size_t i = 0; i < chanName.size(); ++i)
    {
        char c = chanName[i];
        if (c == ' ' || c == ',' || c == '\a')
        {
            srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
            return;
        }
    }

    // チャンネルを探す/作る
    std::map<std::string, Channel>& channels = srv.getChannels();
    if (channels.find(chanName) == channels.end())
    {
        // ない場合は新規作成
        Channel& ch = channels.insert(std::make_pair(chanName, Channel(chanName))).first->second;
        ch.addMember(client.getFd());
        ch.addOperator(client.getFd());
    }
    else
    {
        Channel& ch = channels[chanName];
        // すでにメンバーならreturn
        if (ch.isMember(client.getFd()))
            return;
        // iモードエラーハンドリング
        if (ch.isInviteOnly() && !ch.isInvited(client.getFd()))
        {
            srv.sendToFd(client.getFd(), Replies::ERR_INVITEONLYCHAN(client.getNick(), chanName));
            return;
        }
        // kモードエラーハンドリング
        if (!ch.getKey().empty() && (params.size() < 2 || params[1] != ch.getKey()))
        {
            srv.sendToFd(client.getFd(), Replies::ERR_BADCHANNELKEY(client.getNick(), chanName));
            return;
        }
        // lモードエラーハンドリング
        if (ch.getLimit() > 0 && (static_cast<int>(ch.getMembers().size()) >= ch.getLimit()))
        {
            srv.sendToFd(client.getFd(), Replies::ERR_CHANNELISFULL(client.getNick(), chanName));
            return;
        }
        ch.addMember(client.getFd());
        ch.removeInvited(client.getFd());
    }
    // ここで改めて ch を取得（もう必ず存在する）
    Channel& ch = channels[chanName];
    // 参加成功時のメッセージ
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
    return;
}

void Commands::handlePrivmsg(Server& srv, Client& client, std::vector<std::string>& params)
{
    // validate
    if (!client.isAuthenticated())
        return;
    if (params.empty())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NORECIPIENT(client.getNick(), "PRIVMSG"));
        return;
    }
    if (params.size() < 2 || params[1] == "")
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTEXTTOSEND(client.getNick()));
        return;
    }

    // check target
    std::string target = params[0];
    bool target_is_channel = (target[0] == '#' || target[0] == '&');
    if (target_is_channel)
    {
        // send to channel
        std::map<std::string, Channel>& channels = srv.getChannels();
        std::map<std::string, Channel>::iterator it = channels.find(target);

        if (it == channels.end())
        {
            srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
            return;
        }

        Channel& ch = it->second;
        if (!ch.isMember(client.getFd()))
        {
            srv.sendToFd(client.getFd(), Replies::ERR_CANNOTSENDTOCHAN(client.getNick(), target));
            return;
        }
        std::string msg = ":" + client.getPrefix() + " PRIVMSG " + target + " :" + params[1] + "\r\n";
        srv.sendToChannel(ch, msg, client.getFd());
        return;
    }
    else
    {
        // send to nick
        std::map<int, Client>& clients = srv.getClients();
        for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
        {
            if (it->second.getNick() == target)
            {
                std::string msg = ":" + client.getPrefix() + " PRIVMSG " + target + " :" + params[1] + "\r\n";
                srv.sendToFd(it->second.getFd(), msg);
                return;
            }
        }
    }
    srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
}

void Commands::handlePart(Server& srv, Client& client, std::vector<std::string>& params)
{
    // 未認証なら無視
    if (!client.isAuthenticated())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTREGISTERED(client.getNick()));
        return;
    }

    // paramsが空なら 461 ERR_NEEDMOREPARAMS を返す
    if (params.empty())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(client.getNick(), "PART"));
        return;
    }

    std::string chanName = params[0];
    std::string reason;
    // パラメータがあれば理由の文章も
    if (params.size() > 1)
    {
        reason = params[1];
    }
    else
    {
        reason = "";
    }

    // チャンネルを探す
    std::map<std::string, Channel>& channels = srv.getChannels();
    if (channels.find(chanName) == channels.end())
    {
        // なければ　403エラーを送る
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
        return;
    }

    Channel& ch = channels[chanName];

    // そのチャンネルのメンバーかチェック
    if (!(ch.isMember(client.getFd())))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTONCHANNEL(client.getNick(), chanName));
        return;
    }

    // メッセージを送ってからクライアントを削除
    std::string msg = ":" + client.getPrefix() + " PART " + chanName;
    if (!reason.empty())
    {
        msg += " :" + reason;
    }
    msg += "\r\n";
    srv.sendToChannel(ch, msg);

    ch.removeMember(client.getFd());
    // もしチャンネルから誰もいなくなったら、チャンネルを消す
    if (ch.getMembers().empty())
    {
        channels.erase(chanName);
    }
}

void Commands::handleMode(Server& srv, Client& cli, std::vector<std::string>& params)
{
    // varidate
    if (!cli.isAuthenticated())
    {
        srv.sendToFd(cli.getFd(), Replies::ERR_NOTREGISTERED(cli.getNick()));
        return;
    }
    if (params.empty())
    {
        srv.sendToFd(cli.getFd(), Replies::ERR_NEEDMOREPARAMS(cli.getNick(), "MODE"));
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
    if (params.size() == 1)
    {
        srv.sendToFd(cli.getFd(),
                     Replies::RPL_CHANNELMODEIS(cli.getNick(), ch.getName(), ch.getModeString(), ch.getModeParams()));
        return;
    }

    if (!ch.isOperator(cli.getFd()))
    {
        srv.sendToFd(cli.getFd(), Replies::ERR_CHANOPRIVSNEEDED(cli.getNick(), ch.getName()));
        return;
    }

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
                if (params.size() - 1 < argldx)
                {
                    srv.sendToFd(cli.getFd(), Replies::ERR_NEEDMOREPARAMS(cli.getNick(), "MODE"));
                    break;
                }
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
                if (params.size() - 1 < argldx)
                {
                    srv.sendToFd(cli.getFd(), Replies::ERR_NEEDMOREPARAMS(cli.getNick(), "MODE"));
                    break;
                }
                int limit = atoi(params[argldx++].c_str());
                if (limit > 0)
                {
                    ch.setLimit(limit);
                    recordAppliedMode(appliedModes, lastSign, c, adding);
                    appliedArgs += " " + params[argldx - 1];
                }
                break;
            }
            case 'o':
            {
                if (params.size() - 1 < argldx)
                {
                    srv.sendToFd(cli.getFd(), Replies::ERR_NEEDMOREPARAMS(cli.getNick(), "MODE"));
                    break;
                }
                std::string target_nick = params[argldx++];
                std::map<int, Client>& clients = srv.getClients();
                Client* target_cli = NULL;
                for (std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it)
                {
                    if (it->second.getNick() == target_nick)
                    {
                        target_cli = &it->second;
                        break;
                    }
                }
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

void Commands::handleKick(Server& srv, Client& client, std::vector<std::string>& params)
{
    // 未認証なら無視
    if (!client.isAuthenticated())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTREGISTERED(client.getNick()));
        return;
    }

    // 引数が足りているかチェック
    if (params.size() < 2)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(client.getNick(), "KICK"));
        return;
    }

    std::string chanName = params[0];
    std::string target = params[1];
    std::string reason;
    // パラメータがあれば理由の文章も
    if (params.size() > 2)
    {
        reason = params[2];
    }
    else
    {
        reason = "";
    }

    // チャンネルを探す
    std::map<std::string, Channel>& channels = srv.getChannels();
    if (channels.find(chanName) == channels.end())
    {
        // なければ　403エラーを送る
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
        return;
    }

    Channel& ch = channels[chanName];

    // そのチャンネルのメンバーかチェック
    if (!(ch.isMember(client.getFd())))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTONCHANNEL(client.getNick(), chanName));
        return;
    }

    // 実行者がオペレータかどうかチェック
    if (!ch.isOperator(client.getFd()))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_CHANOPRIVSNEEDED(client.getNick(), chanName));
        return;
    }

    // KICKする対象のfdをサーバーに接続してるか探す
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
    // 見つからなければエラー
    if (targetFd == -1)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
        return;
    }

    // KICK対象がKICKしたいチャンネルメンバーかどうか探す
    if (!ch.isMember(targetFd))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_USERNOTINCHANNEL(client.getNick(), target, chanName));
        return;
    }

    // メッセージを作成・全体に送信
    std::string msg = ":" + client.getPrefix() + " KICK " + chanName + " " + target;
    if (!reason.empty())
    {
        msg += " :" + reason;
    }
    msg += "\r\n";
    srv.sendToChannel(ch, msg);

    // 対象をチャンネルから削除
    ch.removeMember(targetFd);
    if (ch.getMembers().empty())
    {
        channels.erase(chanName);
    }
}

void Commands::handleInvite(Server& srv, Client& client, std::vector<std::string>& params)
{
    // 未認証なら無視
    if (!client.isAuthenticated())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTREGISTERED(client.getNick()));
        return;
    }

    // 引数が足りているかチェック
    if (params.size() < 2)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(client.getNick(), "INVITE"));
        return;
    }

    std::string target = params[0];
    std::string chanName = params[1];

    // チャンネルを探す
    std::map<std::string, Channel>& channels = srv.getChannels();
    if (channels.find(chanName) == channels.end())
    {
        // なければ　403エラーを送る
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
        return;
    }

    Channel& ch = channels[chanName];

    // そのチャンネルのメンバーかチェック
    if (!(ch.isMember(client.getFd())))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTONCHANNEL(client.getNick(), chanName));
        return;
    }

    // オペレーター権限があるかどうかチェック
    if (ch.isInviteOnly() && !(ch.isOperator(client.getFd())))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_CHANOPRIVSNEEDED(client.getNick(), chanName));
        return;
    }

    // INVITEする対象のfdをサーバーに接続してるか探す
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

    // 招待者が存在しなければエラー
    if (targetFd == -1)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
        return;
    }

    // 相手がチャンネルにいるならエラー
    if (ch.isMember(targetFd))
    {
        srv.sendToFd(client.getFd(), Replies::ERR_USERONCHANNEL(client.getNick(), target, chanName));
        return;
    }

    // リストに追加
    ch.addInvited(targetFd);
    // 招待者へ成功通知
    srv.sendToFd(client.getFd(), Replies::RPL_INVITING(client.getNick(), chanName, target));
    // target へ通知
    std::string msg = ":" + client.getPrefix() + " INVITE " + target + " " + chanName + "\r\n";
    srv.sendToFd(targetFd, msg);
}

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

// public
void Commands::dispatch(Server& srv, Client& client, const std::string& line)
{
    typedef void (*HandlerFunc)(Server&, Client&, std::vector<std::string>&);
    static std::map<std::string, HandlerFunc> table;
    if (table.empty())
    {
        table["PING"] = &Commands::handlePing;
        table["PASS"] = &Commands::handlePass;
        table["NICK"] = &Commands::handleNick;
        table["USER"] = &Commands::handleUser;
        table["JOIN"] = &Commands::handleJoin;
        table["PRIVMSG"] = &Commands::handlePrivmsg;
        table["PART"] = &Commands::handlePart;
        table["MODE"] = &Commands::handleMode;
        table["KICK"] = &Commands::handleKick;
        table["INVITE"] = &Commands::handleInvite;
        table["TOPIC"] = &Commands::handleTopic;
    }
    Message msg = parseLine(line);
    std::map<std::string, HandlerFunc>::const_iterator it = table.find(msg.command);
    if (it != table.end())
        it->second(srv, client, msg.params);
}
