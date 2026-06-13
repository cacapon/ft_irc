#include "Commands.hpp"

#include <sys/socket.h>

// private
Commands::Commands()
{
}
Commands::Commands(const Commands& src)
{
    (void)src;
}
Commands& Commands::operator=(const Commands& other)
{
    (void)other;
    return *this;
}
Commands::~Commands()
{
}

void Commands::handlePass(Client& client, std::istringstream& ss, const std::string& password)
{
    std::string pass;
    ss >> pass;
    client.setPassOk((pass == password));
}
void Commands::handleNick(Client& client, std::istringstream& ss)
{
    std::string nick;
    ss >> nick;
    client.setNick(nick);
}
void Commands::handleUser(Client& client, std::istringstream& ss)
{
    std::string user;
    ss >> user;
    client.setUser(user);

    if (client.isAuthenticated())
    {
        std::string reply = ":server 001 " + client.getNick() + " :Welcome!\r\n";
        send(client.getFd(), reply.c_str(), reply.size(), 0);
    }
}

void Commands::handleJoin(Server& srv, Client& client, std::istringstream& ss){
    //未認証なら無視
    if(!client.isAuthenticated()){
        return ;
    }

    //チャンネル名を1つ読む
    std::string chanName;
    ss >> chanName;

    //chanNameがなければ461(ERR_NEEDMOREPARAMS)をclientに送ってreturn
    if(chanName.empty()) {
        //ERR_NEEDMOREPARAMS)
        return ;
    }

    //名前検証(正しくなければERR_BADCHANMASK)
    if(chanName[0] != '#' && chanName[0] != '&') {
        //ERR_BADCHANMASK
        return ;
    }

    //チャンネルを探す/作る
    std::map<std::string, Channel>& channels = srv.getChannels();
    if(channels.find(chanName) == channels.end()){
        //ない場合は新規作成
        Channel& ch = channels.insert(std::make_pair(chanName, Channel(chanName))).first->second;
        ch.addMember(client.getFd());
        ch.addOperator(client.getFd());
    }else{
        Channel& ch = channels[chanName];
        //すでにメンバーならreturn
        if(ch.isMember(client.getFd())){
            return ;
        }
        ch.addMember(client.getFd());
    }
    //ここで改めて ch を取得（もう必ず存在する）
    Channel& ch = channels[chanName];
    //参加成功時のメッセージ
    std::string joinMsg = ":" + client.getPrefix() + " JOIN " + chanName + "\r\n";
    srv.sendToChannel(ch,joinMsg);
    
    //topic
    if(!ch.getTopic().empty()){
        // 332 RPL_TOPIC
    }else {
        //331 RPL_NOTOPIC
    }
    return ;
}

// public
void Commands::dispatch(Server& srv, Client& client, const std::string& line)
{
    std::istringstream ss(line);
    std::string cmd;
    ss >> cmd;
    if (cmd == "PASS")
        handlePass(client, ss, srv.getPassword());
    else if (cmd == "NICK")
        handleNick(client, ss);
    else if (cmd == "USER")
        handleUser(client, ss);
    else if (cmd == "JOIN")
        handleJoin(srv, client, ss);
}
