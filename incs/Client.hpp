#pragma once
#include <string>

class Client
{
private:
    int _fd;
    std::string _nick;
    std::string _user;
    std::string _recvBuf;
    bool _passOk;
    // True while discarding the remainder of a line that already exceeded the
    // RFC 2812 512-byte limit, until the terminating CRLF is seen.
    bool _overLength;

public:
    Client();
    Client(int);
    Client(const Client&);
    Client& operator=(const Client&);
    ~Client();

    int getFd() const;
    std::string getNick() const;
    std::string getUser() const;
    bool isPassOk() const;
    const std::string& getRecvBuf() const;

    void setNick(const std::string& nick);
    void setUser(const std::string& user);
    void setPassOk(bool ok);

    // Since `RecvBuf` is appended using `+=`, use the following instead of a setter:
    void appendRecvBuf(const std::string& data);
    void eraseRecvBuf(size_t pos);
    void clearRecvBuf();

    bool isOverLength() const;
    void setOverLength(bool over);

    bool isAuthenticated() const;

    // For displaying the prefix in participation messages
    std::string getPrefix() const;
};
