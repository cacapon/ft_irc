#include "Commands.hpp"

#include <string>
#include <vector>

#include "Client.hpp"
#include "Replies.hpp"
#include "Server.hpp"

void Commands::handlePass(Server& srv, Client& client, std::vector<std::string>& params)
{
    // Variable for the name of the error reply recipient
    std::string target = client.getNick().empty() ? "*" : client.getNick();

    // An error occurs if the user has already completed registration
    if (client.isAuthenticated())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_ALREADYREGISTRED(target));
        return;
    }

    if (params.empty())
    {
        srv.sendToFd(client.getFd(), Replies::ERR_NEEDMOREPARAMS(target, "PASS"));
        return;
    }
    std::string pass = params[0];
    bool ok = (pass == srv.getPassword());
    client.setPassOk(ok);
    if (!ok)
        srv.sendToFd(client.getFd(), Replies::ERR_PASSWDMISMATCH(target));
    tryRegister(srv, client);
}
