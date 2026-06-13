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

// public
void Commands::dispatch(Client& client, const std::string& line, const std::string& password)
{
	Message msg = parseLine(line);
    if (msg.command == "PASS")
        handlePass(client, msg.params, password);
    else if (msg.command == "NICK")
        handleNick(client, msg.params);
    else if (msg.command == "USER")
        handleUser(client, msg.params);
}