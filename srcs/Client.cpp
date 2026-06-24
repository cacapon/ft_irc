#include "Client.hpp"

Client::Client() : _fd(-1), _passOk(false)
{
}
Client::Client(int fd) : _fd(fd), _passOk(false)
{
}
Client::Client(const Client &src)
    : _fd(src.getFd()), _nick(src.getNick()), _user(src.getUser()), _recvBuf(src.getRecvBuf()), _passOk(src.isPassOk())
{
}
Client &Client::operator=(const Client &other)
{
    if (this != &other)
    {
        _fd = other.getFd();
        _nick = other.getNick();
        _user = other.getUser();
        _recvBuf = other.getRecvBuf();
        _passOk = other.isPassOk();
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
std::string Client::getRecvBuf() const
{
    return (_recvBuf);
}

void Client::setNick(const std::string &nick)
{
    _nick = nick;
}
void Client::setUser(const std::string &user)
{
    _user = user;
}
void Client::setPassOk(bool ok)
{
    _passOk = ok;
}

void Client::appendRecvBuf(const std::string &data)
{
    _recvBuf += data;
}

// \r\nの分はメソッド内で消す
void Client::eraseRecvBuf(size_t pos)
{
    _recvBuf.erase(0, pos + 2);
}

bool Client::isAuthenticated() const
{
    return (_passOk && !_nick.empty() && !_user.empty());
}

std::string Client::getPrefix() const
{
    // irssiは厳密にホスト名を見ているわけではないのでlocalhostにしている（要相談）
    return _nick + "!" + _user + "@localhost";
}
