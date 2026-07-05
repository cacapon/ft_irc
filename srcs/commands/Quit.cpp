#include "Commands.hpp"

#include <string>
#include <vector>

#include "Client.hpp"
#include "Server.hpp"

void Commands::handleQuit(Server&, Client& client, std::vector<std::string>& params)
{
    // Broadcasting and channel removal are centralized in disconnectClient,
    // so here we only record the reason and raise the quit flag.
    std::string reason = params.empty() ? "Client Quit" : params[0];
    client.requestQuit(reason);
}
