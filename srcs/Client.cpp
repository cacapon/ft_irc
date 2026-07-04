#include "Client.hpp"

Client::Client() : _fd(-1), _passOk(false), _overLength(false), _quitRequested(false), _welcomed(false)
{
}
Client::Client(int fd) : _fd(fd), _passOk(false), _overLength(false), _quitRequested(false), _welcomed(false)
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
      _quitReason(src.getQuitReason()),
      _welcomed(src.isWelcomed())
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
        _welcomed = other.isWelcomed();
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

// pos is the index of the line's terminating '\n' (found by the caller via
// find('\n')). Consuming through '\n' handles both '\r\n' and bare '\n' endings.
void Client::eraseRecvBuf(size_t pos)
{
    _recvBuf.erase(0, pos + 1);
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

bool Client::isWelcomed() const
{
    return (_welcomed);
}

void Client::setWelcomed(bool welcomed)
{
    _welcomed = welcomed;
}

bool Client::isAuthenticated() const
{
    return (_passOk && !_nick.empty() && !_user.empty());
}

std::string Client::getPrefix() const
{
    return _nick + "!" + _user + "@" + getHost();
}

std::string Client::getHost() const
{
    // Since irssi doesn't strictly check the hostname, it's set to "localhost".
    // Keep this the single source of the host so the prefix and the 001 host never diverge.
    return "localhost";
}
