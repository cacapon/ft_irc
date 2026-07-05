#include "Channel.hpp"

#include <sstream>

Channel::Channel() : _name(""), _inviteOnly(false), _topicRestricted(false), _limit(0)
{
}
Channel::Channel(const std::string& name) : _name(name), _inviteOnly(false), _topicRestricted(false), _limit(0)
{
}
Channel::Channel(const Channel& other)
    : _name(other._name),
      _members(other._members),
      _operators(other._operators),
      _topic(other._topic),
      _inviteList(other._inviteList),
      _inviteOnly(other._inviteOnly),
      _topicRestricted(other._topicRestricted),
      _key(other._key),
      _limit(other._limit)
{
}
Channel& Channel::operator=(const Channel& other)
{
    if (this != &other)
    {
        _name = other._name;
        _members = other._members;
        _operators = other._operators;
        _topic = other._topic;
        _inviteList = other._inviteList;
        _inviteOnly = other._inviteOnly;
        _topicRestricted = other._topicRestricted;
        _key = other._key;
        _limit = other._limit;
    }
    return *this;
}
Channel::~Channel()
{
}

// member
//  A member function of `std::set`. Returns 0 if `fd` is not in the set.
bool Channel::isMember(int fd) const
{
    return _members.count(fd) != 0;
}
void Channel::addMember(int fd)
{
    _members.insert(fd);
}
void Channel::removeMember(int fd)
{
    _members.erase(fd);
    _operators.erase(fd);
}

// operator
void Channel::addOperator(int fd)
{
    _operators.insert(fd);
}
bool Channel::isOperator(int fd) const
{
    return _operators.count(fd) != 0;
}
void Channel::removeOperator(int fd)
{
    _operators.erase(fd);
}

// invite
bool Channel::isInvited(int fd) const
{
    return _inviteList.count(fd) != 0;
}
void Channel::addInvited(int fd)
{
    _inviteList.insert(fd);
}
void Channel::removeInvited(int fd)
{
    _inviteList.erase(fd);
}

// getters
const std::set<int>& Channel::getMembers() const
{
    return this->_members;
}
const std::set<int>& Channel::getOperators() const
{
    return this->_operators;
}
const std::string& Channel::getTopic() const
{
    return this->_topic;
}
const std::string& Channel::getName() const
{
    return this->_name;
}
bool Channel::isInviteOnly() const
{
    return this->_inviteOnly;
}
bool Channel::isTopicRestricted() const
{
    return this->_topicRestricted;
}
int Channel::getLimit() const
{
    return this->_limit;
}
const std::string& Channel::getKey() const
{
    return this->_key;
}

// setters
void Channel::setTopic(const std::string& topic)
{
    _topic = topic;
}
void Channel::setInviteOnly(bool v)
{
    _inviteOnly = v;
}
void Channel::setTopicRestricted(bool v)
{
    _topicRestricted = v;
}
void Channel::setKey(const std::string& key)
{
    _key = key;
}
void Channel::setLimit(int limit)
{
    _limit = limit;
}

std::string Channel::getModeString() const
{
    std::string modes = "+";
    if (_inviteOnly)
        modes += "i";
    if (_topicRestricted)
        modes += "t";
    if (!_key.empty())
        modes += "k";
    if (_limit > 0)
        modes += "l";
    return modes;
}
std::string Channel::getModeParams() const
{
    std::string result = "";
    if (!_key.empty())
        result += _key;
    if (_limit > 0)
    {
        if (!result.empty())
            result += " ";
        std::ostringstream oss;
        oss << _limit;
        result += oss.str();
    }
    return result;
}
