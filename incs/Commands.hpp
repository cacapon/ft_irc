#pragma once
#include <string>
#include <vector>

#include "Client.hpp"
#include "Server.hpp"

struct Message {
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

    static void handlePass(Client&, std::vector<std::string>&, const std::string&);
    static void handleNick(Client&, std::vector<std::string>&);
    static void handleUser(Client&, std::vector<std::string>&);
    static void handleJoin(Server&, Client&, std::vector<std::string>&);
	static void handlePrivmsg(Server&, Client&, std::vector<std::string>&);
    static void handlePart(Server&, Client&, std::vector<std::string>&);

public:
    static void dispatch(Server&, Client&, const std::string&);
    static Message parseLine(const std::string&);
};
