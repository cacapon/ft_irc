#include "Server.hpp"

#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "Channel.hpp"
#include "Client.hpp"
#include "Commands.hpp"
#include "Utils.hpp"

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
    _serverFd = ft_socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        throw std::runtime_error("socket failed");
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
        throw std::runtime_error("setsockopt failed");
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
        throw std::runtime_error("bind failed");
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
        throw std::runtime_error("listen failed");
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
            throw std::runtime_error("poll failed");

        for (size_t i = 0; i < _pollfds.size(); i++)
        {
            // accept client
            if (_pollfds[i].fd == _serverFd)
            {
                if (_pollfds[i].revents & POLLIN)
                    acceptClient();
                continue;
            }
            // disconnect client
            if (_pollfds[i].revents & (POLLHUP | POLLERR))
            {
                disconnectClient(i, "Connection closed");
                i--;
                continue;
            }
            // receive
            if (_pollfds[i].revents & POLLIN)
            {
                if (receiveData(i))
                {
                    i--;
                    continue;
                }
            }
            // send
            if (_pollfds[i].revents & POLLOUT)
            {
                if (handleWrite(i))
                    i--;
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

void Server::setPollEvent(int fd, short event)
{
    for (size_t i = 0; i < _pollfds.size(); ++i)
    {
        if (_pollfds[i].fd == fd)
        {
            _pollfds[i].events |= event;
            return;
        }
    }
}
void Server::clearPollEvent(int fd, short event)
{
    for (size_t i = 0; i < _pollfds.size(); ++i)
    {
        if (_pollfds[i].fd == fd)
        {
            _pollfds[i].events &= ~event;
            return;
        }
    }
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
    if (clientFd < 0)
        throw std::runtime_error("accept failed");

    std::cout << "New client connected: fd=" << clientFd << std::endl;

    addPollFd(clientFd);
    _clients[clientFd] = Client(clientFd);
    return true;
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
void Server::disconnectClient(size_t i, std::string reason)
{
    int fd = _pollfds[i].fd;
    std::map<int, Client>::iterator clientIt = _clients.find(fd);
    bool notifyChannels = (clientIt != _clients.end() && clientIt->second.isAuthenticated());
    std::string quitMsg;
    if (notifyChannels)
        quitMsg = ":" + clientIt->second.getPrefix() + " QUIT :" + reason + "\r\n";

    // erase(it++) idiom: erasing invalidates it, so post-increment secures the
    // next iterator before the erase (the standard approach for std::map in C++98).
    for (std::map<std::string, Channel>::iterator it = _channels.begin(); it != _channels.end();)
    {
        Channel& ch = it->second;
        if (ch.isMember(fd))
        {
            if (notifyChannels)
                sendToChannel(ch, quitMsg, fd);
            ch.removeMember(fd);
        }
        // PART/KICK remove only the member and not the operator, so an fd may
        // linger in _operators; remove it unconditionally regardless of isMember.
        ch.removeOperator(fd);
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
        // irssi uses '\r\n' while a bare nc (Enter sends only '\n') terminates
        // lines with '\n', so treat the '\n' position as the delimiter and strip
        // a trailing CR from the line for the '\r\n' case.
        while ((pos = client.getRecvBuf().find('\n')) != std::string::npos)
        {
            std::string line = client.getRecvBuf().substr(0, pos);
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            client.eraseRecvBuf(pos);

            // The line terminator closing an over-long line: its head was already
            // truncated and dispatched, so just drop this trailing remainder.
            if (client.isOverLength())
            {
                client.setOverLength(false);
                continue;
            }
            // A whole line exceeding the limit is truncated to 510 (RFC 2812).
            if (line.size() > MAX_CONTENT_LEN)
                line.resize(MAX_CONTENT_LEN);
            Commands::dispatch(*this, client, line);

            // QUIT only raises a flag, so perform the actual disconnect here.
            // After disconnecting, _clients/_pollfds change, so the client
            // reference must not be touched afterwards.
            if (client.isQuitRequested())
            {
                disconnectClient(i, client.getQuitReason());
                return true;
            }
        }

        // The loop above consumed every complete line, so the buffer now holds
        // no line terminator. If what remains still exceeds the limit, it is the
        // head of an over-long line (never a valid command): dispatch its truncated
        // head once, then discard the rest until the terminating LF arrives. This
        // also bounds the buffer against a client that never sends a newline.
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
/**
 * @brief
 *
 * @param i
 * @return true : disconnected
 * @return false : connected
 */
bool Server::handleWrite(size_t i)
{
    int fd = _pollfds[i].fd;
    std::map<int, Client>::iterator clientIt = _clients.find(fd);
    if (clientIt == _clients.end())
        return false;
    Client& client = clientIt->second;
    if (client.getSendBuf().empty())
        return (clearPollEvent(fd, POLLOUT), false);
    const std::string& buf = client.getSendBuf();
    ssize_t n = send(fd, buf.data(), buf.size(), 0);
    if (n > 0)
        client.eraseSendBuf(n);
    if (n < 0)
    {
        if (!(errno == EWOULDBLOCK || errno == EAGAIN))
            return (disconnectClient(i, "Write error"), true);
    }
    if (!client.hasPendingSend())
        clearPollEvent(fd, POLLOUT);
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
    makeSocket();
    addressRecycle();
    bindSocket();
    startListen();
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

Client* Server::findClientByNick(const std::string& nick)
{
    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
    {
        if (it->second.getNick() == nick)
            return &it->second;
    }
    return NULL;
}

Channel* Server::findChannel(const std::string& name)
{
    std::map<std::string, Channel>::iterator it = _channels.find(name);
    if (it == _channels.end())
        return NULL;
    return &it->second;
}

void Server::sendToFd(int fd, const std::string& msg)
{
    queueSend(fd, msg);
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

void Server::queueSend(int fd, const std::string& msg)
{
    std::map<int, Client>::iterator it = _clients.find(fd);
    if (it == _clients.end())
        return;
    it->second.appendSendBuf(msg);
    setPollEvent(fd, POLLOUT);
}
