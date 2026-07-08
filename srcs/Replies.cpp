#include "Replies.hpp"

namespace Replies
{
static std::string makeReply(const std::string& code, const std::string& target, const std::string& params)
{
    return ":" + std::string(SERVER_NAME) + " " + code + " " + target + " " + params + "\r\n";
}

// ==== 001-004: Connection registration ====
std::string RPL_WELCOME(const std::string& nick, const std::string& user, const std::string& host)
{
    return makeReply("001", nick, ":Welcome to the Internet Relay Network " + nick + "!" + user + "@" + host);
}
std::string RPL_YOURHOST(const std::string& nick)
{
    return makeReply("002", nick, ":Your host is " SERVER_NAME ", running version " SERVER_VERSION);
}
std::string RPL_CREATED(const std::string& nick, const std::string& date)
{
    return makeReply("003", nick, ":This server was created " + date);
}
std::string RPL_MYINFO(const std::string& nick)
{
    return makeReply("004", nick, SERVER_NAME " " SERVER_VERSION " o itkol");
}

// ==== 324, 331-341: channel mode / topic / invite ====
std::string RPL_CHANNELMODEIS(const std::string& nick, const std::string& channel, const std::string& mode,
                              const std::string& modeParams)
{
    return makeReply("324", nick, channel + " " + mode + (modeParams.empty() ? "" : " " + modeParams));
}
std::string RPL_NOTOPIC(const std::string& nick, const std::string& channel)
{
    return makeReply("331", nick, channel + " :No topic is set");
}
std::string RPL_TOPIC(const std::string& nick, const std::string& channel, const std::string& topic)
{
    return makeReply("332", nick, channel + " :" + topic);
}
std::string RPL_INVITING(const std::string& nick, const std::string& channel, const std::string& invitedNick)
{
    return makeReply("341", nick, channel + " " + invitedNick);
}

// ==== 353, 366: names ====
std::string RPL_NAMREPLY(const std::string& nick, const std::string& channel, const std::string& names)
{
    return makeReply("353", nick, "= " + channel + " :" + names);
}
std::string RPL_ENDOFNAMES(const std::string& nick, const std::string& channel)
{
    return makeReply("366", nick, channel + " :End of NAMES list");
}

// ==== 4xx: errors ====
std::string ERR_NOSUCHNICK(const std::string& nick, const std::string& target)
{
    return makeReply("401", nick, target + " :No such nick/channel");
}
std::string ERR_NOSUCHCHANNEL(const std::string& nick, const std::string& channel)
{
    return makeReply("403", nick, channel + " :No such channel");
}
std::string ERR_CANNOTSENDTOCHAN(const std::string& nick, const std::string& channel)
{
    return makeReply("404", nick, channel + " :Cannot send to channel");
}
std::string ERR_NORECIPIENT(const std::string& nick, const std::string& command)
{
    return makeReply("411", nick, ":No recipient given (" + command + ")");
}
std::string ERR_NOTEXTTOSEND(const std::string& nick)
{
    return makeReply("412", nick, ":No text to send");
}
std::string ERR_UNKNOWNCOMMAND(const std::string& nick, const std::string& command)
{
    return makeReply("421", nick, command + " :Unknown command");
}
std::string ERR_NONICKNAMEGIVEN(const std::string& nick)
{
    return makeReply("431", nick, ":No nickname given");
}
std::string ERR_ERRONEUSNICKNAME(const std::string& nick, const std::string& badNick)
{
    return makeReply("432", nick, badNick + " :Erroneous nickname");
}
std::string ERR_NICKNAMEINUSE(const std::string& nick, const std::string& badNick)
{
    return makeReply("433", nick, badNick + " :Nickname is already in use");
}
std::string ERR_USERNOTINCHANNEL(const std::string& nick, const std::string& target, const std::string& channel)
{
    return makeReply("441", nick, target + " " + channel + " :They aren't on that channel");
}
std::string ERR_NOTONCHANNEL(const std::string& nick, const std::string& channel)
{
    return makeReply("442", nick, channel + " :You're not on that channel");
}
std::string ERR_USERONCHANNEL(const std::string& nick, const std::string& target, const std::string& channel)
{
    return makeReply("443", nick, target + " " + channel + " :is already on channel");
}
std::string ERR_NOTREGISTERED(const std::string& nick)
{
    return makeReply("451", nick, ":You have not registered");
}
std::string ERR_NEEDMOREPARAMS(const std::string& nick, const std::string& command)
{
    return makeReply("461", nick, command + " :Not enough parameters");
}
std::string ERR_ALREADYREGISTRED(const std::string& nick)
{
    return makeReply("462", nick, ":Unauthorized command (already registered)");
}
std::string ERR_PASSWDMISMATCH(const std::string& nick)
{
    return makeReply("464", nick, ":Password incorrect");
}
std::string ERR_KEYSET(const std::string& nick, const std::string& channel)
{
    return makeReply("467", nick, channel + " :Channel key already set");
}
std::string ERR_CHANNELISFULL(const std::string& nick, const std::string& channel)
{
    return makeReply("471", nick, channel + " :Cannot join channel (+l)");
}
std::string ERR_UNKNOWNMODE(const std::string& nick, char modeChar, const std::string& channel)
{
    return makeReply("472", nick, std::string(1, modeChar) + " :is unknown mode char to me for " + channel);
}
std::string ERR_INVITEONLYCHAN(const std::string& nick, const std::string& channel)
{
    return makeReply("473", nick, channel + " :Cannot join channel (+i)");
}
std::string ERR_BADCHANNELKEY(const std::string& nick, const std::string& channel)
{
    return makeReply("475", nick, channel + " :Cannot join channel (+k)");
}
std::string ERR_CHANOPRIVSNEEDED(const std::string& nick, const std::string& channel)
{
    return makeReply("482", nick, channel + " :You're not channel operator");
}
}  // namespace Replies
