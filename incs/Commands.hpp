#pragma once
#include <sstream>
#include <string>

#include "Client.hpp"

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

public:
    static void dispatch(Client&, const std::string&, const std::string&);
};