#pragma once

#include <poll.h>

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include "Channel.hpp"
#include "Client.hpp"

class Server
{
private:  // Properties
    int _serverFd;
    int _port;
    std::string _password;
    std::vector<struct pollfd> _pollfds;
    std::map<int, Client> _clients;
    std::map<std::string, Channel> _channels;

private:  // Methods
    bool makeSocket();
    bool addressRecycle();
    bool makeNonBlocking();
    bool bindSocket();
    bool startListen();
    bool pollLoop();

    // helper methods
    void addPollFd(int);
    void setPollEvent(int, short);
    void clearPollEvent(int, short);

    // pollLoop Utils.
    bool acceptClient();
    void disconnectClient(size_t i, std::string reason);
    bool receiveData(size_t);
    bool handleWrite(size_t);

public:
    Server();
    Server(int, const std::string&);
    Server(const Server&);
    Server& operator=(const Server&);
    ~Server();
    void run();

    // Function to send a notification to the entire channel via broadcast
    // getter
    std::map<int, Client>& getClients();
    std::map<std::string, Channel>& getChannels();
    const std::string& getPassword() const;

    void sendToFd(int fd, const std::string& msg);
    void sendToChannel(const Channel& ch, const std::string& msg, int excludeFd = -1);
    void queueSend(int, const std::string&);
};
