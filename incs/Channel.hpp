#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <set>
#include <string>

class Channel
{
private:
    std::string _name;
    std::set<int> _members;    // 接続してるクライアントのソケットファイルのfdを保管
    std::set<int> _operators;  // ニックネームは変更可能なため、不変のfdで保存
    std::string _topic;
    std::set<int> _inviteList;
    bool _inviteOnly;
    bool _topicRestricted;
    std::string _key;
    int _limit;

public:
    // OCF
    Channel();
    Channel(const std::string& name);
    Channel(const Channel& other);
    Channel& operator=(const Channel& other);
    ~Channel();

    // member
    bool isMember(int fd) const;
    void addMember(int fd);
    void removeMember(int fd);

    // operator
    void addOperator(int fd);
    bool isOperator(int fd) const;
    void removeOperator(int fd);

    // invite
    bool isInvited(int fd) const;
    void addInvited(int fd);
    void removeInvited(int fd);

    // getters
    const std::set<int>& getMembers() const;
    const std::set<int>& getOperators() const;
    const std::string& getTopic() const;
    const std::string& getName() const;
    bool isInviteOnly() const;
    bool isTopicRestricted() const;
    int getLimit() const;
    const std::string& getKey() const;

    // setters
    void setTopic(const std::string& topic);
    void setInviteOnly(bool v);
    void setTopicRestricted(bool v);
    void setKey(const std::string& key);
    void setLimit(int limit);

    // mode string (RPL_CHANNELMODEIS)
    std::string getModeString() const;
    std::string getModeParams() const;
};

#endif
