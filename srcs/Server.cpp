#include "Server.hpp"
#include "Signal.hpp"
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sstream>
#include <algorithm>
#include <csignal>
#include <cerrno>

volatile sig_atomic_t g_running = 1;

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
    std::cout << "Server destructor called" << std::endl;

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

    std::cout << "Listening to port " << port << std::endl;
}

void signalHandler(int sig)
{
    (void)sig;
    g_running = 0;
}

void Server::flushClient(int idx)
{
    Client* client = NULL;
    int fd = _fds[idx].fd;
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i].getFd() == fd)
        {
            client = &_clients[i];
            break;
        }
    }

    if (!client || !client->hasPendingWrite())
        return;

    const std::string& buf = client->getWriteBuffer();
    ssize_t sent = send(fd, buf.c_str(), buf.size(), 0);
    if (sent > 0)
        client->consumeWriteBuffer(static_cast<size_t>(sent));

    if (!client->hasPendingWrite())
        _fds[idx].events &= ~POLLOUT;
}

void Server::checkClientTimeouts() {
    std::time_t now = std::time(NULL);
    size_t i = 0;

    while (i < _clients.size()) {
        Client& c = _clients[i];

        if (!c.isRegistered()) {
            i++;
            continue;
        }

        if (c.getLastPingSent() == 0) {
            if (now - c.getLastActivity() >= PING_INTERVAL) {
                sendReply(c,"PING :ircserv");
                c.setLastPingSent(now);
            }
            i++;
        }

        else {
            if (now - c.getLastPingSent() >= PING_TIMEOUT) {
                std::cout << "Client (fd=" << c.getFd() << ", nick=" << c.getNickname()
                            << ") timed out (no PONG received)" << std::endl;
                int fd = c.getFd();
                for (size_t j = 0; j < _fds.size(); j++) {
                    if (_fds[j].fd == fd) {
                        removeClient((int)j);
                        break;
                    }
                }
            
            } else {
                i++; 
            }
        }
    }
}

void Server::run() {

    while (g_running)
    {
        /*poll() surveille tous les Fds et bloque jusqu'a ce qu'au moins un soit pret. 
        Timout -1 = attend idefiniment.
        C'est le coeur du serveur : une seule fonction gere tous les clients*/
        int ready = poll(_fds.data(), _fds.size(), 1000);
        
        if (ready < 0)
        {
            if (!g_running)
                break;
            throw std::runtime_error("poll() failed");
        }
        
        /*Parcours tous les Fds pour trouver ceux qui sont prets*/
        for (size_t i = 0; i < _fds.size(); i++)
        {
            /*revents est rempli par poll() - POLLIN = donnees dispo
            Si ce Fd n'a rien a lire, on passe au suivant*/
            if ((_fds[i].revents & POLLIN))
            {
                if (_fds[i].fd == _serverFd)
                    acceptClient(); /*New connexion entrante*/
                else
                    handleClient(i); /*Donnees d'un client existant*/
            }
            if (i < _fds.size() && (_fds[i].revents & POLLOUT))
                flushClient(i);
        }
        checkClientTimeouts();
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
        if (bytes == 0)
            std::cout << "Client disconnected (fd=" << clientFd << ")" << std::endl;
        else
            std::cerr << "recv( error on fd=" << clientFd << " )" << std::endl;

        for (size_t i = 0; i < _clients.size(); ++i)
        {
            if (_clients[i].getFd() == clientFd)
            {
                if (!_clients[i].getNickname().empty())
                {
                    std::string quitLine = ":" + _clients[i].getNickname() + "!" + _clients[i].getUsername() + "@localhost QUIT :Connection closed";
                    for (size_t j = 0; j < _clients.size(); ++j)
                    {
                        if (_clients[j].getFd() != clientFd)
                            sendReply(_clients[j], quitLine);
                    }
                }
                break;
            }
        }
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
    _clients[cidx].setLastActivity(std::time(NULL));

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
    int fd = _fds[idx].fd;
    for (size_t i = 0; i < _channels.size(); i++)
        _channels[i].removeMember(fd);
    size_t i = 0;
    while (i < _channels.size()) {
        if (_channels[i].getMemberCount() == 0)
            _channels.erase(_channels.begin() + i);
        else
            i++;
    }
    close(fd);
    _fds.erase(_fds.begin() + idx);
    for (size_t j = 0; j < _clients.size(); ++j) {
        if (_clients[j].getFd() == fd) {
            _clients.erase(_clients.begin() + j);
            break;
        }
    }
}

static std::vector<std::string> splitline(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token)
        tokens.push_back(token);
    return tokens;
}


void Server::sendReply(Client& client, const std::string& msg) {
    client.appendToWriteBuffer(msg + "\r\n");

    for (size_t i = 0; i < _fds.size(); i++)
    {
        if (_fds[i].fd == client.getFd())
        {
            _fds[i].events |= POLLOUT;
            break;
        }
    }
}

void Server::broadcastToChannel(Channel& chan, const std::string& msg, int excludeFd) {
    std::vector<int> members = chan.getMembers();
    for (size_t i = 0; i < members.size(); i++) {
        if (members[i] == excludeFd)
            continue;
        for (size_t j = 0; j < _clients.size(); j++) {
            if (_clients[j].getFd() == members[i]) {
                sendReply(_clients[j], msg);
                break;
            }
        }
    }
}

void Server::dispatch(Client& client, const std::string& line) {
    std::vector<std::string> tokens = splitline(line);
    if (tokens.empty())
        return;
    
    std::string command = tokens[0];
    for (size_t i = 0; i < command.size(); i++)
        command[i] = toupper(command[i]);
    std::vector<std::string> params(tokens.begin() + 1, tokens.end());

    if (command == "CAP") {
        if (!params.empty() && params[0] == "LS")
            sendReply(client, ":ircserv CAP * LS :");
        return;
    }

    std::cout << "Command: " << command << std::endl;

    if (_commands.count(command) == 0)
    {
        std::cout << "Unknown command: " << command << std::endl;
        return;
    }

    (this->*_commands[command])(client, params);
}


