#include "Client.hpp"

Client::Client() : _fd(-1), _passOk(false), _overLength(false), _quitRequested(false)
{
}
Client::Client(int fd) : _fd(fd), _passOk(false), _overLength(false), _quitRequested(false)
{
}
Client::Client(const Client& src)
    : _fd(src.getFd()),
      _nick(src.getNick()),
      _user(src.getUser()),
      _recvBuf(src.getRecvBuf()),
      _passOk(src.isPassOk()),
      _overLength(src.isOverLength()),
      _quitRequested(src.isQuitRequested()),
      _quitReason(src.getQuitReason())
{
}
Client& Client::operator=(const Client& other)
{
    if (this != &other)
    {
        _fd = other.getFd();
        _nick = other.getNick();
        _user = other.getUser();
        _recvBuf = other.getRecvBuf();
        _passOk = other.isPassOk();
        _overLength = other.isOverLength();
        _quitRequested = other.isQuitRequested();
        _quitReason = other.getQuitReason();
    }
    return (*this);
}
Client::~Client()
{
}

int Client::getFd() const
{
    return (_fd);
}
std::string Client::getNick() const
{
    return (_nick);
}
std::string Client::getUser() const
{
    return (_user);
}
bool Client::isPassOk() const
{
    return (_passOk);
}
const std::string& Client::getRecvBuf() const
{
    return (_recvBuf);
}

void Client::setNick(const std::string& nick)
{
    _nick = nick;
}
void Client::setUser(const std::string& user)
{
    _user = user;
}
void Client::setPassOk(bool ok)
{
    _passOk = ok;
}

void Client::appendRecvBuf(const std::string& data)
{
    _recvBuf += data;
}

// Removes the \r\n along with the consumed portion
void Client::eraseRecvBuf(size_t pos)
{
    _recvBuf.erase(0, pos + 2);
}

void Client::clearRecvBuf()
{
    _recvBuf.clear();
}

bool Client::isOverLength() const
{
    return (_overLength);
}

void Client::setOverLength(bool over)
{
    _overLength = over;
}

void Client::requestQuit(const std::string& reason)
{
    _quitRequested = true;
    _quitReason = reason;
}

bool Client::isQuitRequested() const
{
    return (_quitRequested);
}

const std::string& Client::getQuitReason() const
{
    return (_quitReason);
}

bool Client::isAuthenticated() const
{
    return (_passOk && !_nick.empty() && !_user.empty());
}

std::string Client::getPrefix() const
{
    // Since irssi doesn't strictly check the hostname, I've set it to “localhost”
    return _nick + "!" + _user + "@localhost";
}
