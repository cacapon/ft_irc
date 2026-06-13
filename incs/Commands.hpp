#pragma once
#include <sstream>
#include <string>

#include "Client.hpp"
#include "Server.hpp"

class Commands
{
private:  // Methods
    Commands();
    Commands(const Commands&);
    Commands& operator=(const Commands&);
    ~Commands();

    static void handlePass(Client&, std::istringstream&, const std::string&);
    static void handleNick(Client&, std::istringstream&);
    static void handleUser(Client&, std::istringstream&);

    //JOIN
    static void handleJoin(Server&, Client&, std::istringstream&);

public:
    static void dispatch(Server&, Client&, const std::string&);
};
