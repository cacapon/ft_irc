#include "Server.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

#include "Client.hpp"

// private

/**
 * @brief make socket.
 *
 * @return bool
 * @note AF_INET: Address Family INET = IPv4 protocol.
 * @note SOCK_STREAM: USE TCP. Order Guaranteed and Reliable.
 */
bool Server::makeSocket()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        return (perror("socket"), false);
    return true;
}

/**
 * @brief Configure settings to reuse addresses.
 *
 * @return bool
 * @note SOL_SOCKET: Specify the protocol layer, SOL_SOCKET is socket layer.
 * @note SO_REUSEADDR: Once configured, you will be able to bind to that port even while in the TIME_WAIT state.
 * @note Since the type of the third argument is unknown, you must also specify the size of that type in the fourth
 * argument.
 */
bool Server::addressRecycle()
{
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        return (perror("setsockopt"), false);
    return true;
}
/**
 * @brief set Non-Blocking fd.
 *
 * @return bool
 * @note F_SETFL: Set File status FLags.
 * @note O_NONBLOCK: Flag to enable non-blocking mode.
 */
bool Server::makeNonBlocking()
{
    if (fcntl(_serverFd, F_SETFL, O_NONBLOCK) < 0)
        return (perror("fcntl"), false);
    return true;
}

/**
 * @brief Bind the socket.
 *
 * @return bool
 * @note sin_family: Socket INternet FAMILY.
 * @note sin_addr: Socket INternet ADDRess. INADDR_ANY = IN ADDRess ANY
 * @note sin_port: Socket INternet PORT.
 * @note htons: Host TO Network Short.
 * 			A function to convert to big-endian format in order to pass the correct port number.
 */
bool Server::bindSocket()
{
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);
    if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return (perror("bind"), false);
    return true;
}
/**
 * @brief Start listening on the port.
 *
 * @return int 0:
 */
bool Server::startListen()
{
    if (listen(_serverFd, SOMAXCONN) < 0)
        return (perror("listen"), false);
    std::cout << "Server listening on port " << _port << std::endl;
    return true;
}
/**
 * @brief start poll Loop.
 * 
 * @return true 
 * @return false 
 */
bool Server::pollLoop()
{
    struct pollfd pfd;
    pfd.fd = _serverFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);

    while (true)
    {
        int ret = poll(&_pollfds[0], _pollfds.size(), -1);
        if (ret < 0)
        {
            perror("poll");
            return false;
        }

        for (size_t i = 0; i < _pollfds.size(); i++)
        {
            if (_pollfds[i].revents & POLLIN)
            {
                if (_pollfds[i].fd == _serverFd)
                {
                    int clientFd = accept(_serverFd, NULL, NULL);
                    if (clientFd >= 0)
                    {
                        std::cout << "New client connected: fd=" << clientFd << std::endl;

                        struct pollfd clientPfd;
                        clientPfd.fd = clientFd;
                        clientPfd.events = POLLIN;
                        clientPfd.revents = 0;
                        _pollfds.push_back(clientPfd);
                        _clients[clientFd] = Client(clientFd);
                    }
                }
                else
                {
                    char buf[512];
                    int bytes = recv(_pollfds[i].fd, buf, sizeof(buf) - 1, 0);
                    if (bytes <= 0)
                    {
                        std::cout << "Client disconnected: fd=" << _pollfds[i].fd << std::endl;
                        close(_pollfds[i].fd);
                        _clients.erase(_pollfds[i].fd);
                        _pollfds.erase(_pollfds.begin() + i);
                        i--;
                    }
                    else
                    {
                        int fd = _pollfds[i].fd;
                        Client& client = _clients[fd];
                        client.appendRecvBuf(std::string(buf, bytes));

                        size_t pos;
                        while ((pos = client.getRecvBuf().find("\r\n")) != std::string::npos)
                        {
                            std::string line = client.getRecvBuf().substr(0, pos);
                            client.eraseRecvBuf(pos);
                            std::istringstream ss(line);
                            std::string cmd;
                            ss >> cmd;
                            if (cmd == "PASS")
                            {
                                std::string pass;
                                ss >> pass;
                                client.setPassOk((pass == _password));
                            }
                            else if (cmd == "NICK")
                            {
                                std::string nick;
                                ss >> nick;
                                client.setNick(nick);
                            }
                            else if (cmd == "USER")
                            {
                                std::string user;
                                ss >> user;
                                client.setUser(user);

                                if (client.isAuthenticated())
                                {
                                    std::string reply = ":server 001 " + client.getNick() + " :Welcome!\r\n";
                                    send(fd, reply.c_str(), reply.size(), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

// public
Server::Server() : _serverFd(-1), _port(0)
{
}
Server::Server(int port, const std::string& password) : _serverFd(-1), _port(port), _password(password)
{
}
Server::Server(const Server& src)
    : _serverFd(src._serverFd),
      _port(src._port),
      _password(src._password),
      _pollfds(src._pollfds),
      _clients(src._clients)
{
}
Server& Server::operator=(const Server& src)
{
    if (&src != this)
    {
        _serverFd = src._serverFd;
        _port = src._port;
        _password = src._password;
        _pollfds = src._pollfds;
        _clients = src._clients;
    }
    return *this;
}
Server::~Server()
{
}

void Server::run()
{
    if (!makeSocket())
        return;
    if (!addressRecycle())
        return;
    if (!makeNonBlocking())
        return;
    if (!bindSocket())
        return;
    if (!startListen())
        return;
    pollLoop();
}