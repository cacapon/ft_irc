#pragma once

#include <sstream>
#include <string>

#define SERVER_NAME "ircserv"
#define SERVER_VERSION "1.0"

namespace Replies
{
// ==== 001-004: Connection registration ====
std::string RPL_WELCOME(const std::string& nick, const std::string& user, const std::string& host);
std::string RPL_YOURHOST(const std::string& nick);
std::string RPL_CREATED(const std::string& nick, const std::string& date);
std::string RPL_MYINFO(const std::string& nick);

// ==== 324, 331-341: channel mode / topic / invite ====
std::string RPL_CHANNELMODEIS(const std::string& nick, const std::string& channel, const std::string& mode,
                              const std::string& modeParams);
std::string RPL_NOTOPIC(const std::string& nick, const std::string& channel);
std::string RPL_TOPIC(const std::string& nick, const std::string& channel, const std::string& topic);
std::string RPL_INVITING(const std::string& nick, const std::string& channel, const std::string& invitedNick);

// ==== 353, 366: names ====
std::string RPL_NAMREPLY(const std::string& nick, const std::string& channel, const std::string& names);
std::string RPL_ENDOFNAMES(const std::string& nick, const std::string& channel);

// ==== 4xx: errors ====
std::string ERR_NOSUCHNICK(const std::string& nick, const std::string& target);
std::string ERR_NOSUCHCHANNEL(const std::string& nick, const std::string& channel);
std::string ERR_CANNOTSENDTOCHAN(const std::string& nick, const std::string& channel);
std::string ERR_NORECIPIENT(const std::string& nick, const std::string& command);
std::string ERR_NOTEXTTOSEND(const std::string& nick);
std::string ERR_UNKNOWNCOMMAND(const std::string& nick, const std::string& command);
std::string ERR_NONICKNAMEGIVEN(const std::string& nick);
std::string ERR_ERRONEUSNICKNAME(const std::string& nick, const std::string& badNick);
std::string ERR_NICKNAMEINUSE(const std::string& nick, const std::string& badNick);
std::string ERR_USERNOTINCHANNEL(const std::string& nick, const std::string& target, const std::string& channel);
std::string ERR_NOTONCHANNEL(const std::string& nick, const std::string& channel);
std::string ERR_USERONCHANNEL(const std::string& nick, const std::string& target, const std::string& channel);
std::string ERR_NOTREGISTERED(const std::string& nick);
std::string ERR_NEEDMOREPARAMS(const std::string& nick, const std::string& command);
std::string ERR_ALREADYREGISTRED(const std::string& nick);
std::string ERR_PASSWDMISMATCH(const std::string& nick);
std::string ERR_KEYSET(const std::string& nick, const std::string& channel);
std::string ERR_CHANNELISFULL(const std::string& nick, const std::string& channel);
std::string ERR_UNKNOWNMODE(const std::string& nick, char modeChar, const std::string& channel);
std::string ERR_INVITEONLYCHAN(const std::string& nick, const std::string& channel);
std::string ERR_BADCHANNELKEY(const std::string& nick, const std::string& channel);
std::string ERR_CHANOPRIVSNEEDED(const std::string& nick, const std::string& channel);
}  // namespace Replies
