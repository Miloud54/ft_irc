#include "Server.hpp"

void Server::cmdPass(Client& client, std::vector<std::string>& params) {
    if (client.isRegistered())
    {
        sendReply(client, ":ircserv 462 * :You are already registered");
        return;
    }
    if (params.empty())
    {
        sendReply(client, ":ircserv 461 * PASS :Not enough parameters");
        return;
    }
    if (params[0] != _password)
    {
        sendReply(client, ":ircserv 464 * :Incorrect password");
        return;        
    }
    client.setPassOk(true);
} 


static bool isValidNickname(const std::string& nickname) {
    if (nickname.empty())
        return false;
    for (size_t i = 0; i < nickname.size(); ++i)
    {
        char c = nickname[i];
        if (c == ' ' || c == '#' || c == ':')
            return false;
    }
    return true;
}

void Server::cmdNick(Client& client, std::vector<std::string>& params) {
    if (!client.isPassOk())
    {
        sendReply(client, ":ircserv 451 * :PASS is not validated yet");
        return;
    }

    if (params.empty())
    {
        sendReply(client, ":ircserv 431 * :No nickname given");
        return;
    }

    if (!isValidNickname(params[0]))
    {
        sendReply(client, ":ircserv 432 * " + params[0] + " Erroneous nickname");
        return;
    }

    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i].getNickname() == params[0])
        {
            sendReply(client, ":ircserv 433 * " + params[0] + " :Nickname already in use");
            return;        
        }
    }
    
    std::string oldNick = client.getNickname();
    client.setNickname(params[0]);
    client.setNickOk(true);
    
    if (!oldNick.empty() && client.isRegistered())
    {
        std::string nickChangeMsg = ":" + oldNick + "!" + client.getUsername() + "@localhost NICK :" + client.getNickname();
        sendReply(client, nickChangeMsg); 
        
        for (size_t i = 0; i < _channels.size(); ++i)
        {
            if (_channels[i].hasMember(client.getFd()))
                broadcastToChannel(_channels[i], nickChangeMsg, client.getFd());
        }
        return;
    }
    
    if (client.isPassOk() && client.isNickOk() && client.isUserOk())
    {
        client.setRegistered(true);
        sendReply(client, ":ircserv 001 " + client.getNickname() + " :Welcome to the IRC server " + client.getNickname());
    }
}

void Server::cmdUser(Client& client, std::vector<std::string>& params) {
    if (client.isRegistered())
    {
        sendReply(client, ":ircserv 462 * :You are already registered");
        return;
    }
    if (params.size() < 4)
    {
        sendReply(client, ":ircserv 461 * USER :Not enough parameters");
        return;
    }
    if (!client.isPassOk())
    {
        sendReply(client, ":ircserv 451 * :PASS is not validated yet");
        return;
    }
    client.setUsername(params[0]);
    
    std::string realname;
    for (size_t i = 3; i < params.size(); ++i)
    {
        if (i > 3)
            realname += " ";
        if (!params[i].empty() && params[i][0] == ':')
            realname += params[i].substr(1);
        else
            realname += params[i];
    }
    client.setRealname(realname);
    client.setUserOk(true);
    if (client.isPassOk() && client.isNickOk() && client.isUserOk())
    {
        client.setRegistered(true);
        sendReply(client, ":ircserv 001 " + client.getNickname() + " :Welcome to the IRC server " + client.getNickname());
    }
}

void Server::cmdQuit(Client& client, std::vector<std::string>& params) {
    std::string reason = "Client Quit";
    if(!params.empty()) 
    {
        std::string r;
        for (size_t i = 0; i < params.size(); ++i)
        {
            if (i)
                r += " ";
            if (!params[i].empty() && params[i][0] == ':')
                r += params[i].substr(1);
            else
                r += params[i];
        }
        if (!r.empty())
            reason = r;
    }
    
    std::string sender;
    if (!client.getNickname().empty())
        sender = client.getNickname();
    else
        sender = client.getUsername();
    
    std::string quitLine = ":" + sender + "!" + client.getUsername() + "@localhost QUIT :" + reason;

    sendReply(client, quitLine); 

    for (size_t i = 0; i < _clients.size(); ++i)
    {
        if (_clients[i].getFd() == client.getFd())
            continue;
        sendReply(_clients[i], quitLine);
    }

    size_t fdIdx = 0;
    for (size_t j = 0; j < _fds.size(); ++j)
    {
        if (_fds[j].fd == client.getFd())
        {
            fdIdx = j;
            break;
        }
    }
    if (fdIdx < _fds.size())
    {
        removeClient(static_cast<int>(fdIdx));
    }
}

void Server::cmdCap(Client& client, std::vector<std::string>& params)
{
    (void)client;
    (void)params;
}