#pragma once

#include <poll.h>
#include <map>
#include <string>
#include <vector>

#include "Client.hpp"

class Server
{
private: // Properties
    int _serverFd;
    int _port;
    std::string _password;
    std::vector<struct pollfd> _pollfds;
    std::map<int, Client> _clients;

private: // Methods
    bool makeSocket();
    bool addressRecycle();
    bool makeNonBlocking();
    bool bindSocket();
    bool startListen();
    bool pollLoop();

	// pollLoop Utils.
	bool acceptClient();
	void disconnectClient(size_t);

public:
    Server();
	Server(int, const std::string&);
    Server(const Server&);
    Server& operator=(const Server&);
    ~Server();
    void run();
};