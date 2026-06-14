#include "Commands.hpp"

#include <sys/socket.h>
#include <cstddef>
#include <string>
#include <vector>

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
	Message msg = parseLine(line);
    if (msg.command == "PASS")
        handlePass(client, msg.params, srv.getPassword());
    else if (msg.command == "NICK")
        handleNick(client, msg.params);
    else if (msg.command == "USER")
        handleUser(client, msg.params);
    else if (cmd == "JOIN")
        handleJoin(srv, client, ss);
}
