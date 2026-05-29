/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: edidier <edidier@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 16:13:02 by edidier           #+#    #+#             */
/*   Updated: 2026/05/29 17:06:40 by edidier          ###   ########.fr       */
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
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

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
        throw std::runtime_error("bind() failed");

    if (listen(_serverFd, 10) < 0)
        throw std::runtime_error("listen() failed");

    
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
    int bytes = recv(_fds[idx].fd, buf, sizeof(buf) - 1, 0);
    if (bytes <= 0)
    {
        /*0 = deco propre (client a ferme la connexion)
        <0 = erreur reseau (connexion interrompue brutalement)*/
        if (bytes == 0)
            std::cout << "Client disconnected (fd=" << _fds[idx].fd << ")" << std::endl;
        else
            std::cerr << "recv( error on fd=" << _fds[idx].fd << " )" << std::endl;
        removeClient(idx);
        return;
    }

    std::string data(buf, bytes);
    _clients[idx - 1].appendToBuffer(data);

    std::string line;
    while(!(line = _clients[idx - 1].extractLine()).empty())
        dispatch(_clients[idx - 1], line);

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
    for (size_t j; j < _fds.size(); ++j)
    {
        if (_fds[j].fd == client.getFd())
        {
            fdIdxm = j;
            break;
        }
    }
    removeClient(fdIdx);
}

void Server::cmdPong(Client& client, std::vector<std::string>& params) {
    (void)client;
    (void)params;
}