#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>

class Channel {
private:
    std::string _name;
    std::set<int> _members;//接続してるクライアントのソケットファイルのfdを保管
    std::set<int> _operators; //ニックネームは変更可能なため、不変のfdで保存
    std::string _topic;

public:
    //OCF
    Channel();
    Channel(const std::string& name);
    Channel(const Channel& other);
    Channel& operator=(const Channel& other);
    ~Channel();

    bool isMember(int fd) const;
    void addMember(int fd);
    void addOperator(int fd);
    bool isOperator(int fd) const;
    const std::set<int>& getMembers() const;
    const std::string& getTopic() const;
    const std::string& getName() const;
};

#endif
