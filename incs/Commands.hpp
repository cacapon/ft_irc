#pragma once
#include <string>
#include <vector>

#include "Client.hpp"

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

public:
    static Message parseLine(const std::string&);
    static void dispatch(Client&, const std::string&, const std::string&);
};