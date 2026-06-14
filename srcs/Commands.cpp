#include "Commands.hpp"

#include <sys/socket.h>
#include <cstddef>
#include <string>
#include <vector>
#include "Channel.hpp"
#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

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

/**
 * @brief In accordance with the IRC2810-2.3.1 specifications, 
the command is parsed into prefix, command, and params.
 * 
 * @param line 
 * @return Message 
 */
Message Commands::parseLine(const std::string &line) {
	Message msg;
	size_t pos = 0;

	// get prefix
	if (!line.empty() && line[0] == ':')
	{
		size_t end = line.find(' ');
		if (end == std::string::npos)
		{
			msg.prefix = line.substr(1);
			pos = line.size();
		}
		else
		{
			msg.prefix = line.substr(1, end -1);
			pos = end + 1;
		}
	}

	// get command
	while (pos < line.size() && line[pos] == ' ') pos++;
	size_t start = pos;
	while (pos < line.size() && line[pos] != ' ') pos++;
	msg.command = line.substr(start, pos - start);

	// get params
	while (pos < line.size()) 
	{
		while (pos < line.size() && line[pos] == ' ') pos++;
		if (pos >= line.size()) break;

		if (line[pos] == ':')
		{
			msg.params.push_back(line.substr(pos + 1));
			break;
		}
		start = pos;
		while (pos < line.size() && line[pos] != ' ') pos++;
		msg.params.push_back(line.substr(start, pos - start));
	}
	return msg;
}

void Commands::handlePass(Client& client,  std::vector<std::string>& params, const std::string& password)
{
	if (params.empty())
	{
		return;
	}
    std::string pass = params[0];
    client.setPassOk((pass == password));
}
void Commands::handleNick(Client& client, std::vector<std::string>& params)
{
	if (params.empty())
	{
		return;
	}
    std::string nick = params[0];
    client.setNick(nick);
}
void Commands::handleUser(Client& client, std::vector<std::string>& params)
{
	if (params.empty())
	{
		return; 
	}
    std::string user = params[0];
    client.setUser(user);

    if (client.isAuthenticated())
    {
        std::string reply = ":server 001 " + client.getNick() + " :Welcome!\r\n";
        send(client.getFd(), reply.c_str(), reply.size(), 0);
    }
}

void Commands::handleJoin(Server& srv, Client& client, std::vector<std::string>& params){
    //未認証なら無視
    if(!client.isAuthenticated()){
        srv.sendToFd(client.getFd(), Replies::ERR_NOTREGISTERED(client.getNick()));
        return ;
    }
    //chanNameがなければ461(ERR_NEEDMOREPARAMS)をclientに送ってreturn
	if(params.empty()){
		srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(client.getNick(), "JOIN"));
		return ;
	}

    //チャンネル名を1つ読む
    std::string chanName = params[0];

    //名前検証
    //先頭が#, & 以外　OR　チャンネル名が1文字もない　(ERR_NOSUCHCHANNEL)
    if(chanName.size() < 2 || (chanName[0] != '#' && chanName[0] != '&')) {
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
        return ;
    }
    //チャンネル名に使われている文字が正しいか
    for(size_t i = 0; i < chanName.size(); ++i){
        char c = chanName[i];
        if(c == ' ' || c == ',' || c == '\a') {
            srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
            return ;
        }
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
        srv.sendToFd(client.getFd(), Replies::RPL_TOPIC(client.getNick(), chanName, ch.getTopic()));
    }else {
        srv.sendToFd(client.getFd(), Replies::RPL_NOTOPIC(client.getNick(), chanName));
    }
    return ;
}

void Commands::handlePrivmsg(Server& srv, Client& client, std::vector<std::string>& params){
    // validate
    if(!client.isAuthenticated())
        return ;
    if(params.empty()) {
        srv.sendToFd(client.getFd(), Replies::ERR_NORECIPIENT(client.getNick(), "PRIVMSG"));
        return ;
    }
    if (params.size() < 2 || params[1] == "") {
        srv.sendToFd(client.getFd(), Replies::ERR_NOTEXTTOSEND(client.getNick()));
        return ;
    }

    // check target
    std::string target = params[0];
    bool target_is_channel = (target[0] == '#' || target[0] == '&');
    if (target_is_channel) {
        // send to channel
        std::map<std::string, Channel>& channels = srv.getChannels();
        std::map<std::string, Channel>::iterator it = channels.find(target);
        
        if (it == channels.end()) {
            srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
            return ;
        }

        Channel& ch = it->second;
        if (!ch.isMember(client.getFd())) {
            srv.sendToFd(client.getFd(), Replies::ERR_CANNOTSENDTOCHAN(client.getNick(), target));
            return ;
        }
        std::string msg = ":" + client.getPrefix() + " PRIVMSG " + target + " :" + params[1] + "\r\n";
        srv.sendToChannel(ch, msg, client.getFd());
        return ;
    }
    else {
        // send to nick
        std::map<int, Client>& clients = srv.getClients();
        for(std::map<int, Client>::iterator it = clients.begin(); it != clients.end(); ++it){
            if (it->second.getNick() == target)
            {
                std::string msg = ":" + client.getPrefix() + " PRIVMSG " + target + " :" + params[1] + "\r\n";
                srv.sendToFd(it->second.getFd(), msg);
                return ;
            }
        }
    }
    srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHNICK(client.getNick(), target));
}

void Commands::handlePart(Server& srv, Client& client,std::vector<std::string>& params){
    //未認証なら無視
    if(!client.isAuthenticated()){
        srv.sendToFd(client.getFd(), Replies::ERR_NOTREGISTERED(client.getNick()));
        return ;
    }
    
    //paramsが空なら 461 ERR_NEEDMOREPARAMS を返す
    if(params.empty()){
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(client.getNick(), "PART"));
        return ;
    }

    std::string chanName = params[0];
    std::string reason;
    //パラメータがあれば理由の文章も
    if(params.size() > 1){
        reason = params[1];
    } else {
        reason = "";
    }

    //チャンネルを探す
    std::map<std::string, Channel>& channels = srv.getChannels();
    if(channels.find(chanName) == channels.end()){
        //なければ　403エラーを送る
        srv.sendToFd(client.getFd(), Replies::ERR_NOSUCHCHANNEL(client.getNick(), chanName));
        return ;
    } 
    
    Channel& ch = channels[chanName];

    //そのチャンネルのメンバーかチェック
    if(!(ch.isMember(client.getFd()))){
        srv.sendToFd(client.getFd(), Replies::ERR_NOTONCHANNEL(client.getNick(), chanName));
        return ;
    }

    //メッセージを送ってからクライアントを削除
    std::string msg = ":" + client.getPrefix() + " PART " + chanName;
    if(!reason.empty()){
        msg += " :" + reason;
    }
    msg += "\r\n";
    srv.sendToChannel(ch, msg);
    
    ch.removeMember(client.getFd());
    //もしチャンネルから誰もいなくなったら、チャンネルを消す
    if(ch.getMembers().empty()){
        channels.erase(chanName);
    }
}

// public
void Commands::dispatch(Server& srv, Client& client, const std::string& line)
{
	Message msg = parseLine(line);
    if (msg.command == "PASS")
        handlePass(client, msg.params, srv.getPassword());
    else if (msg.command == "NICK")
        handleNick(client, msg.params);
    else if (msg.command == "USER")
        handleUser(client, msg.params);
    else if (msg.command == "JOIN")
        handleJoin(srv, client, msg.params);
    else if (msg.command == "PRIVMSG")
        handlePrivmsg(srv, client, msg.params);
    else if (msg.command == "PART")
        handlePart(srv, client, msg.params);
}
