#include "Server.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "Channel.hpp"
#include "Client.hpp"
#include "Commands.hpp"

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
    addPollFd(_serverFd);

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
                    acceptClient();
                else
                {
                    if (receiveData(i))
                        i--;
                }
            }
        }
    }
    return true;
}

void Server::addPollFd(int fd)
{
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _pollfds.push_back(pfd);
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool Server::acceptClient()
{
    int clientFd = accept(_serverFd, NULL, NULL);
    if (clientFd >= 0)
    {
        std::cout << "New client connected: fd=" << clientFd << std::endl;

        addPollFd(clientFd);
        _clients[clientFd] = Client(clientFd);
        return true;
    }
    return (perror("accept"), false);
}

/**
 * @brief Disconnect client. Removes the client from every channel it belongs
 * to and broadcasts QUIT before closing the fd, since a closed fd can be
 * reused by the OS for the next accept() and would otherwise let the new
 * connection inherit the previous client's channel membership/operator status.
 *
 * @param i index into _pollfds of the client being disconnected
 * @param reason QUIT reason shown to other channel members
 */
void Server::disconnectClient(size_t i, const std::string& reason)
{
    int fd = _pollfds[i].fd;
    std::map<int, Client>::iterator clientIt = _clients.find(fd);
    bool notifyChannels = (clientIt != _clients.end() && clientIt->second.isAuthenticated());
    std::string quitMsg;
    if (notifyChannels)
        quitMsg = ":" + clientIt->second.getPrefix() + " QUIT :" + reason + "\r\n";

    // erase(it++) イディオム: 削除後は it が無効化されるため、post-increment で
    // 次のイテレータを先に確保してから erase する (C++98 の std::map に対する定石)。
    for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end();)
    {
        Channel& ch = it->second;
        if (ch.isMember(fd))
        {
            if (notifyChannels)
                sendToChannel(ch, quitMsg, fd);
            ch.removeMember(fd);
            ch.removeOperator(fd);
        }
        ch.removeInvited(fd);

        if (ch.getMembers().empty())
            _channels.erase(it++);
        else
            ++it;
    }

    std::cout << "Client disconnected: fd=" << fd << std::endl;
    close(fd);
    _clients.erase(fd);
    _pollfds.erase(_pollfds.begin() + i);
}

/**
 * @brief
 *
 * @param i
 * @return true : disconnected
 * @return false: continue
 */
bool Server::receiveData(size_t i)
{
    char buf[512];
    int bytes = recv(_pollfds[i].fd, buf, sizeof(buf) - 1, 0);
    if (bytes <= 0)
    {
        disconnectClient(i, "Connection closed");
        return true;
    }
    else
    {
        // RFC 2812 2.3: a message is at most 512 bytes including the trailing
        // CRLF, leaving at most 510 bytes for the command and its parameters.
        const size_t MAX_MSG_LEN = 512;
        const size_t MAX_CONTENT_LEN = MAX_MSG_LEN - 2;  // 510

        int fd = _pollfds[i].fd;
        Client& client = _clients[fd];
        client.appendRecvBuf(std::string(buf, bytes));

        size_t pos;
        while ((pos = client.getRecvBuf().find("\r\n")) != std::string::npos)
        {
            std::string line = client.getRecvBuf().substr(0, pos);
            client.eraseRecvBuf(pos);

            // The CRLF closing an over-long line: its head was already truncated
            // and dispatched, so just drop this trailing remainder.
            if (client.isOverLength())
            {
                client.setOverLength(false);
                continue;
            }
            // A whole line exceeding the limit is truncated to 510 (RFC 2812).
            if (line.size() > MAX_CONTENT_LEN)
                line.resize(MAX_CONTENT_LEN);
            Commands::dispatch(*this, client, line);

            // QUIT はフラグを立てるだけなので、ここで実際の切断処理を行う。
            // 切断後は _clients/_pollfds が変化するため client 参照には触れない。
            if (client.isQuitRequested())
            {
                disconnectClient(i, client.getQuitReason());
                return true;
            }
        }

        // The loop above consumed every complete line, so the buffer now holds
        // no CRLF. If what remains still exceeds the limit, it is the head of an
        // over-long line (never a valid command): dispatch its truncated head
        // once, then discard the rest until CRLF arrives. This also bounds the
        // buffer against a client that never sends CRLF.
        if (client.getRecvBuf().size() > MAX_MSG_LEN)
        {
            if (!client.isOverLength())
            {
                Commands::dispatch(*this, client, client.getRecvBuf().substr(0, MAX_CONTENT_LEN));
                client.setOverLength(true);
            }
            client.clearRecvBuf();
        }
    }
    return false;
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
    // Sending a message to a disconnected socket causes the OS to kill the process,
    // so SIGPIPE must be ignored to keep the server running.
    signal(SIGPIPE, SIG_IGN);
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

// Broadcast Notifications
//  getter
std::map<int, Client>& Server::getClients()
{
    return _clients;
}

std::map<std::string, Channel>& Server::getChannels()
{
    return _channels;
}

const std::string& Server::getPassword() const
{
    return _password;
}

void Server::sendToFd(int fd, const std::string& msg)
{
    send(fd, msg.c_str(), msg.size(), 0);
}

void Server::sendToChannel(const Channel& ch, const std::string& msg, int excludeFd)
{
    const std::set<int>& members = ch.getMembers();
    // Get the fds of all channel members and send msg to each of them
    for (std::set<int>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        if (*it == excludeFd)
            continue;
        sendToFd(*it, msg);
    }
}
