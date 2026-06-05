/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: edidier <edidier@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 16:13:02 by edidier           #+#    #+#             */
/*   Updated: 2026/06/05 14:34:46 by bde-la-p         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <algorithm>

struct ChannelNameEquals {
    std::string name;

    ChannelNameEquals(const std::string& channelName) : name(channelName) {}

    bool operator()(const Channel& channel) const {
        return channel.getName() == name;
    }
};

Server::Server(int port, const std::string& password) : _password(password) {
    _fds.reserve(64);

    _commands["PASS"] = &Server::cmdPass;
    _commands["NICK"] = &Server::cmdNick;
    _commands["USER"] = &Server::cmdUser;
    _commands["QUIT"] = &Server::cmdQuit;
    _commands["PING"] = &Server::cmdPing;
    _commands["PONG"] = &Server::cmdPong;
    _commands["PRIVMSG"] = &Server::cmdPrivmsg;
    _commands["NOTICE"] = &Server::cmdNotice;
    _commands["MODE"] = &Server::cmdMode;
    _commands["JOIN"] = &Server::cmdJoin;
    _commands["PART"] = &Server::cmdPart;
    _commands["TOPIC"] = &Server::cmdTopic;
    _commands["KICK"] = &Server::cmdKick;
    _commands["INVITE"] = &Server::cmdInvite;
    
    setupSocket(port);
}

Server::~Server() {
    for (size_t i = 0; i < _fds.size(); i++)
        close(_fds[i].fd);
}

void Server::setupSocket(int port) {
    /*Cree socket TCP (SOCK_STREAM = flux fiable, AF_INET = IPv4)*/
    /*Retourne un Fd (tout est fichier sous Unix)*/
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0)
        throw std::runtime_error("socket() failed"); 

    /*SO_REUSADDR permet de relancer serveur immediatement apres un arret
    sans attendre expiration du TIME_WAIT (env 60s)*/
    int opt = 1;
    if (setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
	close(_serverFd);
	throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
    }

    /*Mode non-bloquant : les appels comme accept() et recv() retournent immediatement
    avec -1 (EAGAIN) s'ils n'ont rien a faire au lieu de bloquer le programme*/
    fcntl(_serverFd, F_SETFL, O_NONBLOCK);

    /*Config l'addresse d'ecoute*/
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
	close(_serverFd);
        throw std::runtime_error("bind() failed");
    }

    if (listen(_serverFd, 10) < 0)
    {   
	close(_serverFd); 
    	throw std::runtime_error("listen() failed");
    }
    
    struct pollfd pfd;
    pfd.fd = _serverFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);

    std::cout << "Listenning to port " << port << std::endl;
}

void Server::run() {
    while (true)
    {
        /*poll() surveille tous les Fds et bloque jusqu'a ce qu'au moins un soit pret. 
        Timout -1 = attend idefiniment.
        C'est le coeur du serveur : une seule fonction gere tous les clients*/
        int ready = poll(_fds.data(), _fds.size(), -1);
        if (ready < 0)
            throw std::runtime_error("poll() failed");
        
        /*Parcours tous les Fds pour trouver ceux qui sont prets*/
        for (size_t i = 0; i < _fds.size(); i++)
        {
            /*revents est rempli par poll() - POLLIN = donnees dispo
            Si ce Fd n'a rien a lire, on passe au suivant*/
            if (!(_fds[i].revents & POLLIN))
                continue;

            if (_fds[i].fd == _serverFd)
                acceptClient(); /*New connexion entrante*/
            else
                handleClient(i); /*Donnees d'un client existant*/
        }
    }
}

void Server::acceptClient() {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    /*accept() extrait la 1ere connexion de la file d'attente et retourne un new Fd dedie a ce client.
    Le serveurfd continue d'ecouter les nouvelles connexions*/
    int clientFd = accept(_serverFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) 
        throw std::runtime_error("accept() failed");

    /*Mode non-bloquant*/
    fcntl(clientFd, F_SETFL, O_NONBLOCK);

    /*Ajoute nouveau client a la liste surveillee par poll()*/
    struct pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    _fds.push_back(pfd);
    _clients.push_back(Client(clientFd));

    std::cout << "New client connected (fd=" << clientFd << ")" << std::endl;
}

void Server::handleClient(int idx) {
    char buf[BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
  
    /*revc() lit les donnee dispo sur ce Fd
    Retourne : nb d'octets lus, 0 si deco propre, -1 si erreur*/
    int clientFd = _fds[idx].fd;
    int bytes = recv(_fds[idx].fd, buf, sizeof(buf) - 1, 0);
    if (bytes <= 0)
    {
        /*0 = deco propre (client a ferme la connexion)
        <0 = erreur reseau (connexion interrompue brutalement)*/
        if (bytes == 0)
            std::cout << "Client disconnected (fd=" << clientFd << ")" << std::endl;
        else
            std::cerr << "recv( error on fd=" << clientFd << " )" << std::endl;
        removeClient(idx);
        return;
    }

    int cidx = -1;
    for (size_t i = 0; i < _clients.size(); ++i) {
        if (_clients[i].getFd() == clientFd) {
            cidx = static_cast<int>(i);
            break;
        }
    }
    if (cidx < 0)
        return;

    _clients[cidx].appendToBuffer(std::string(buf, bytes));

    while (true) {
        cidx = -1;
        for (size_t i = 0; i < _clients.size(); ++i) {
            if (_clients[i].getFd() == clientFd) {
                cidx = static_cast<int>(i);
                break;
            }
        }
        if (cidx < 0)
            break;

        std::string line = _clients[cidx].extractLine();
        if (line.empty())
            break;

        dispatch(_clients[cidx], line);
    }

}

void Server::removeClient(int idx) {
    close(_fds[idx].fd);
    _fds.erase(_fds.begin() + idx);
    _clients.erase(_clients.begin() + (idx - 1));
    
}

static std::vector<std::string> splitline(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token)
        tokens.push_back(token);
    return tokens;
}

void Server::dispatch(Client& client, const std::string& line) {
    std::vector<std::string> tokens = splitline(line);
    if (tokens.empty())
        return;
    
    std::string command = tokens[0];
    std::vector<std::string> params(tokens.begin() + 1, tokens.end());
    std::cout << "Command: " << command << std::endl;

    if (_commands.count(command) == 0)
    {
        std::cout << "Unknown command: " << command << std::endl;
        return;
    }

    (this->*_commands[command])(client, params);
}

void Server::sendReply(Client& client, const std::string& msg) {
    std::string reply = msg + "\r\n";
    send(client.getFd(), reply.c_str(), reply.size(), 0);
}

void Server::cmdCap(Client& client, std::vector<std::string>& params)
{
    (void)client;
    (void)params;
}

void Server::cmdPass(Client& client, std::vector<std::string>& params) {
    if (client.isRegistered())
    {
        sendReply(client, ": ircserv 462 * :You are already registered\r\n");
        return;
    }
    if (params.empty()) 
    {
        sendReply(client, ": ircserv 461 * PASS :Not enough parameters\r\n");
        return;        
    }
    if (params[0] != _password) 
    {
        sendReply(client, ": ircserv 464 * :Incorrect password\r\n");
        return;        
    }
    client.setPassOk(true);
} 

void Server::cmdNick(Client& client, std::vector<std::string>& params) {
    if (!client.isPassOk())
    {
        sendReply(client, ": ircserv 451 * :PASS is not validated yet\r\n");
        return;
    }
    if (params.empty()) 
    {
        sendReply(client, ": ircserv 431 * PASS :No nickname given\r\n");
        return;        
    }
    for (size_t  i = 0; i < _clients.size(); i++)
    {
        if (_clients[i].getNickname() == params[0]) 
        {
            sendReply(client, ": ircserv 433 * " + params[0] + " :Nickname already in use\r\n");
            return;        
        }
    }
    client.setNickname(params[0]);
    client.setNickOk(true);
    if (client.isPassOk() && client.isNickOk() && client.isUserOk())
    {
        client.setRegistered(true);
        sendReply(client, ":ircserv 001 " + client.getNickname() + " :Welcome to the IRC server " + client.getNickname());
    }
}

void Server::cmdUser(Client& client, std::vector<std::string>& params) {
    if (client.isRegistered())
    {
        sendReply(client, ": ircserv 462 * :You are already registered\r\n");
        return;
    }
    if (params.size() < 4) 
    {
        sendReply(client, ": ircserv 461 * USER :Not enough parameters\r\n");
        return;        
    }
    if (!client.isPassOk())
    {
        sendReply(client, ": ircserv 451 * :PASS is not validated yet\r\n");
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
    
    std::string quitLine = std::string(":") + sender + " QUIT :" + reason;

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

void Server::cmdPing(Client& client, std::vector<std::string>& params) {
    if (params.empty()) {
        sendReply(client, ":ircserv 409 " + client.getNickname() + " :No origin specified");
        return;
    }
    std::string token = params[0];

    if (!token.empty() && token[0] == ':')
        token = token.substr(1);

    sendReply(client, "PONG :" + token);
}


void Server::cmdPong(Client& client, std::vector<std::string>& params) {
    (void)client;
    (void)params;
}

void Server::cmdPrivmsg(Client& client, std::vector<std::string>& params) {
    if (!client.isRegistered()) {
        sendReply(client, ":ircserv 451 :You have not registered");
        return;
    }
    if (params.empty()) {
        sendReply(client, ":ircserv 411 " + client.getNickname() + " :No recipient given (PRIVMSG)");
        return;
    }
     if (params.size() < 2) {
        sendReply(client, ":ircserv 411 :No recipient given (PRIVMSG)");
        return;
    }

    std::string target = params[0];
    std::string message = buildTrailing(params, 1);

    if (!message.empty() && message[0] == '\x01') {
        if (target[0] == '#') {
            Channel *chan = findChannelByName(target);
            if (!chan) {
                sendReply(client, ":ircserv 403 " + client.getNickname() + " " + target + " :No such channel");
                return;
            }
            chan->broadcastMessage(":" + client.getNickname() + " PRIVMSG " + target + " :" + message + "\r\n", client.getFd());
        }
        else {
            Client* recipient = findClientByNick(target);
            if (!recipient) {
                sendReply(client, ":ircserv 401 " + client.getNickname() + " " + target + " :No such nick");
                return;
            }
            sendReply(*recipient, ":" + client.getNickname() + " PRIVMSG " + recipient->getNickname() + " :" + message);
        }
        return;
    }
    
    if (target[0] == '#') {
        Channel *chan = findChannelByName(target);
        if (!chan) {
            sendReply(client, ":ircserv 403 " + client.getNickname() + " " + target + " :No such channel");
            return;
        }
       chan->broadcastMessage(":" + client.getNickname() + " PRIVMSG " + target + " :" + message + "\r\n", client.getFd());
    }
    else
    {
        Client* recipient = findClientByNick(target);
        if (!recipient)
        {
            sendReply(client, ":ircserv 401 " + client.getNickname() + " " + target + " :No such nick");
            return;
        }
        sendReply(*recipient, ":" + client.getNickname() + " PRIVMSG " + recipient->getNickname() + " :" + message);
    }
}

void Server::cmdNotice(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered() || params.size() < 2)
        return;

    std::string target = params[0];
    std::string message = buildTrailing(params, 1);

    if (!message.empty() && message[0] == '\x01') {
        if (target[0] == '#') {
            Channel* chan = findChannelByName(target);
            if (chan)
                chan->broadcastMessage(":" + client.getNickname() + "!" + client.getUsername() + "@localhost NOTICE " + target + " :" + message + "\r\n", client.getFd());
        } else {
            Client* recipient = findClientByNick(target);
            if (recipient)
                sendReply(*recipient, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost NOTICE " + target + " :" + message);
        }
        return;
    }

    if (target[0] == '#') {
        Channel* chan = findChannelByName(target);
        if (chan)
            chan->broadcastMessage(":" + client.getNickname() + "!" + client.getUsername() + "@localhost NOTICE " + target + " :" + message + "\r\n", client.getFd());
    } else {
        Client* recipient = findClientByNick(target);
        if (recipient)
            sendReply(*recipient, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost NOTICE " + target + " :" + message);
    }
}

void Server::cmdMode(Client& client, std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendReply(client, ":ircserv 461 " + client.getNickname() + " MODE :Not enough parameters");
        return;
    }

    std::string target = params[0];
    if (target[0] != '#')
        return;
    
    Channel *chan = findChannelByName(target);
    if (!chan)
    {
        sendReply(client, ":ircserv 403 " + client.getNickname() + " " + target + " :No such channel");
        return;
    }

    if (params.size() == 1)
    {
        std::string modeStr = "+";
        if (chan->isInviteOnly())
            modeStr += "i";
        if (chan->isTopicRestricted())
            modeStr += "t";
        if (chan->isKeyEnabled())
            modeStr += "k";
        if (chan->isUserLimitEnabled())
            modeStr += "l";
        sendReply(client, ":ircserv 324 " + client.getNickname() + " " + target + " " + modeStr);
        return;
    }

    if (!chan->isOperator(client.getFd()))
    {
        sendReply(client, ":ircserv 482 " + client.getNickname() + " " + target + " :You're not channel operator");
        return;
    }

    std::string modeStr = params[1];
    size_t paramIdx = 2;
    char sign = '+';

    for (size_t i = 0; i < modeStr.size(); i++)
    {
        char m = modeStr[i];
        if (m == '+'|| m == '-')
        {
            sign = m;
            continue;
        }
        if (m == 'i')
            chan->setInviteOnly(sign == '+');
        else if (m == 't')
            chan->setTopicRestricted(sign == '+');
        else if (m == 'k')
        {
            if (sign == '+' && paramIdx < params.size())
                chan->setKey(params[paramIdx++]);
            else
                chan-> clearKey();
        }
        else if (m == 'o')
        {
            if (paramIdx < params.size())
            {
                Client *op = findClientByNick(params[paramIdx++]);
                if (op && chan->hasMember(op->getFd()))
                {
                    if (sign == '+')
                        chan->addOperator(op->getFd());
                    else
                        chan->removeOperator(op->getFd());
                }
            }
        }
        else if (m == 'l')
        {
            if (sign == '+' && paramIdx < params.size())
            {
                std::istringstream iss(params[paramIdx++]);
                size_t limit;
                iss >> limit;
                chan->setUserLimit(limit);
            }
            else
                chan->clearUserLimit();
        }
    }
    std::string echo = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost MODE " + target;
    for (size_t i = 1; i < params.size(); i++)
        echo += " " + params[i];
    chan-> broadcastMessage(echo + "\r\n");
}


void Server::cmdJoin(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered()) {
        sendReply(client, ":ircserv 451 :You have not registered");
        return;
    }
    if (params.empty()) {
        sendReply(client, ":ircserv 461 JOIN :Not enough parameters");
        return;
    }
    std::string channelName = params[0];
    std::string key = (params.size() > 1) ? params[1] : "";
    Channel* chan = findChannelByName(channelName);
    if (!chan) {
        _channels.push_back(Channel(channelName));
        Channel& created = _channels.back();
        _nameToChannel[channelName] = &created;
        chan = &created;
    }

    if (chan->isKeyEnabled()) {
        if (key.empty() || key != chan->getKey()) {
            sendReply(client, ":ircserv 475 " + client.getNickname() + " " + channelName + " :Cannot join channel (bad key)");
            return;
        }
    }

    if (!chan->canJoin(client.getFd())) {
        if (chan->isInviteOnly())
            sendReply(client, ":ircserv 473 " + client.getNickname() + " " + channelName + " :Cannot join channel (invite only)");
        else if (chan->isUserLimitEnabled())
            sendReply(client, ":ircserv 471 " + client.getNickname() + " " + channelName + " :Cannot join channel (channel is full)");
        else
            sendReply(client, ":ircserv 471 " + client.getNickname() + " " + channelName + " :Cannot join channel");
        return;
    }

    if (!chan->addMember(client.getFd())) {
        sendReply(client, ":ircserv 443 " + client.getNickname() + " " + channelName + " :is already on channel");
        return;
    }

    std::string joinMsg = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost JOIN " + channelName;
    chan->broadcastMessage(joinMsg + "\r\n");

    if (!chan->getTopic().empty())
        sendReply(client, ":ircserv 332 " + client.getNickname() + " " + channelName + " :" + chan->getTopic());
    else
        sendReply(client, ":ircserv 331 " + client.getNickname() + " " + channelName + " :No topic is set");
}

void Server::cmdPart(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered())
    {
        sendReply(client, ":ircserv 451 :You have not registered");
        return ;
    }
    if (params.empty())
    {
        sendReply(client, ":ircserv 461 PART :Not enough parameters");
        return ;
    }
    std::string channelName = params[0];
    Channel* chan = findChannelByName(channelName);
    if (!chan)
    {
        sendReply(client, ":ircserv 403 " + channelName + " :No such channel");
        return ;
    }
    if (!chan->hasMember(client.getFd()))
    {
        sendReply(client, ":ircserv 442 " + channelName + " :You're not on that channel");
        return ;
    }
    std::string message = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PART " + channelName;
    chan->broadcastMessage(message + "\r\n", client.getFd());
    chan->removeMember(client.getFd());
    if (chan->getMemberCount() == 0)
    {
        std::string channelNameToErase = chan->getName();
        _nameToChannel.erase(channelNameToErase);
        _channels.erase(std::remove_if(_channels.begin(), _channels.end(), ChannelNameEquals(channelNameToErase)), _channels.end());
    }
}

void Server::cmdTopic(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered()) {
        sendReply(client, ":ircserv 451 :You have not registered");
        return;
    }
    if (params.empty()) {
        sendReply(client, ":ircserv 461 TOPIC :Not enough parameters");
        return;
    }
    std::string channelName = params[0];
    Channel* chan = findChannelByName(channelName);
    if (!chan) {
        sendReply(client, ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel");
        return;
    }
    if (params.size() == 1) {
        if (chan->getTopic().empty())
            sendReply(client, ":ircserv 331 " + client.getNickname() + " " + channelName + " :No topic is set");
        else
            sendReply(client, ":ircserv 332 " + client.getNickname() + " " + channelName + " :" + chan->getTopic());
        return;
    }

    if (chan->isTopicRestricted() && !chan->isOperator(client.getFd())) {
        sendReply(client, ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not channel operator");
        return;
    }

    std::string newTopic = buildTrailing(params, 1);
    chan->setTopic(newTopic);
    std::string echo = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost TOPIC " + channelName + " :" + newTopic;
    chan->broadcastMessage(echo + "\r\n");
}

void Server::cmdKick(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered()) {
        sendReply(client, ":ircserv 451 :You have not registered");
        return;
    }
    if (params.size() < 2) {
        sendReply(client, ":ircserv 461 KICK :Not enough parameters");
        return;
    }
    std::string channelName = params[0];
    std::string targetNick = params[1];
    std::string reason = (params.size() > 2) ? buildTrailing(params, 2) : "Kicked";

    Channel* chan = findChannelByName(channelName);
    if (!chan) {
        sendReply(client, ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel");
        return;
    }
    if (!chan->isOperator(client.getFd())) {
        sendReply(client, ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not channel operator");
        return;
    }
    Client* target = findClientByNick(targetNick);
    if (!target) {
        sendReply(client, ":ircserv 441 " + client.getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }
    if (!chan->hasMember(target->getFd())) {
        sendReply(client, ":ircserv 441 " + client.getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }

    std::string kickMsg = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost KICK " + channelName + " " + targetNick + " :" + reason;
    chan->broadcastMessage(kickMsg + "\r\n");
    chan->removeMember(target->getFd());
    if (chan->getMemberCount() == 0) {
        std::string channelNameToErase = chan->getName();
        _nameToChannel.erase(channelNameToErase);
        _channels.erase(std::remove_if(_channels.begin(), _channels.end(), ChannelNameEquals(channelNameToErase)), _channels.end());
    }
}

void Server::cmdInvite(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered()) {
        sendReply(client, ":ircserv 451 :You have not registered");
        return;
    }
    if (params.size() < 2) {
        sendReply(client, ":ircserv 461 INVITE :Not enough parameters");
        return;
    }
    std::string targetNick = params[0];
    std::string channelName = params[1];
    Client* target = findClientByNick(targetNick);
    if (!target) {
        sendReply(client, ":ircserv 401 " + client.getNickname() + " " + targetNick + " :No such nick");
        return;
    }
    Channel* chan = findChannelByName(channelName);
    if (!chan) {
        sendReply(client, ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel");
        return;
    }
    if (!chan->hasMember(client.getFd())) {
        sendReply(client, ":ircserv 442 " + client.getNickname() + " " + channelName + " :You're not on that channel");
        return;
    }
    if (chan->isInviteOnly() && !chan->isOperator(client.getFd())) {
        sendReply(client, ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not channel operator");
        return;
    }
    chan->invite(target->getFd());
    sendReply(*target, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost INVITE " + targetNick + " :" + channelName);
    sendReply(client, ":ircserv 341 " + client.getNickname() + " " + targetNick + " " + channelName);
}
