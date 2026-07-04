#include "Commands.hpp"

#include <string>
#include <vector>

#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

void Commands::handlePing(Server& srv, Client& cli, std::vector<std::string>& params)
{
    // PING is a connection-keepalive check and creates no session state, so it
    // is answered regardless of registration (no PASS required).
    std::string token = params.empty() ? "" : params[0];
    std::string msg = ":" SERVER_NAME " PONG " SERVER_NAME " :" + token + "\r\n";
    srv.sendToFd(cli.getFd(), msg);
}
