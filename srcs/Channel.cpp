#include "Channel.hpp"


Channel::Channel() : _name(""){}
Channel::Channel(const std::string& name) : _name(name){}
Channel::Channel(const Channel& other): _name(other._name), _members(other._members),_operators(other._operators), _topic(other._topic){}
Channel& Channel::operator=(const Channel& other){ 
    if(this != &other){
        _name = other._name;
        _members = other._members;
        _operators = other._operators;
        _topic = other._topic;
    }
    return *this;
}
Channel::~Channel(){}

bool Channel::isMember(int fd) const{
     return _members.count(fd) != 0;//std::setのメンバ関数。配列内にfdがなければ0を返す
}
void Channel::addMember(int fd){
     _members.insert(fd);
}
void Channel::addOperator(int fd){
      _operators.insert(fd);
}
bool Channel::isOperator(int fd) const{
      return _operators.count(fd) != 0;
}
const std::set<int>& Channel::getMembers() const{
      return this->_members;
}
const std::string& Channel::getTopic() const{
      return this->_topic;
}
const std::string& Channel::getName() const{
      return this->_name;
}
