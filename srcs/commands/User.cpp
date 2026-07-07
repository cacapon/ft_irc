#include "Commands.hpp"

#include <string>
#include <vector>

#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

void Commands::handleUser(Server& srv, Client& client, std::vector<std::string>& params)
{
    // Variable for the name of the error reply recipient
    std::string target = replyTarget(client);

    // An error occurs if the user has already completed registration
    if (client.isAuthenticated())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_ALREADYREGISTRED(target));
        return;
    }

    if (params.size() < 4)
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(target, "USER"));
        return;
    }
    std::string user = params[0];
    client.setUser(user);

    tryRegister(srv, client);
}
