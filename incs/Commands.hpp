#pragma once
#include <string>
#include <vector>

#include "Client.hpp"
#include "Server.hpp"

struct Message
{
    std::string prefix;
    std::string command;
    std::vector<std::string> params;
};

class Commands
{
private:  // Methods
    Commands();
    Commands(const Commands&);
    Commands& operator=(const Commands&);
    ~Commands();

    static void recordAppliedMode(std::string& appliedModes, char& lastSign, char modeChar, bool adding);
    static bool isValidNick(const std::string& nick);
    // Shared step called at the end of each handler so that registration is
    // detected whenever PASS/NICK/USER complete, in any order.
    static void tryRegister(Server& srv, Client& client);

    // Nick for numeric replies; "*" before the nickname is set.
    static std::string replyTarget(const Client& client);
    // Each guard sends the numeric reply and returns false when the check fails.
    static bool requireRegistered(Server& srv, Client& client);
    static bool requireParams(Server& srv, Client& client, const std::vector<std::string>& params, size_t minParams,
                               const std::string& command);
    static bool requireMembership(Server& srv, Client& client, Channel& ch, const std::string& chanName);
    static bool requireChanOp(Server& srv, Client& client, Channel& ch, const std::string& chanName);
    // Sends ERR_NOSUCHCHANNEL and returns NULL when the channel does not exist.
    static Channel* findChannelOrReply(Server& srv, Client& client, const std::string& chanName);
    // Shared broadcast for KICK/PART: "<command> <targetPart>[ :<reason>]".
    static void broadcastWithOptionalReason(Server& srv, Channel& ch, Client& client, const std::string& command,
                                             const std::string& targetPart, const std::string& reason);

    static void handlePing(Server&, Client&, std::vector<std::string>&);
    static void handlePass(Server&, Client&, std::vector<std::string>&);
    static void handleNick(Server&, Client&, std::vector<std::string>&);
    static void handleUser(Server&, Client&, std::vector<std::string>&);
    static void handleJoin(Server&, Client&, std::vector<std::string>&);
    static void handlePrivmsg(Server&, Client&, std::vector<std::string>&);
    static void handlePart(Server&, Client&, std::vector<std::string>&);
    static void handleMode(Server&, Client&, std::vector<std::string>&);
    static void handleKick(Server&, Client&, std::vector<std::string>&);
    static void handleInvite(Server&, Client&, std::vector<std::string>&);
    static void handleTopic(Server&, Client&, std::vector<std::string>&);
    static void handleQuit(Server&, Client&, std::vector<std::string>&);

public:
    static void dispatch(Server&, Client&, const std::string&);
    static Message parseLine(const std::string&);
};
