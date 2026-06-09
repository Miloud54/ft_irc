#pragma once
#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include <ctime>
#include "Client.hpp"
#include "Channel.hpp"
#include "parser_msg_irc.hpp"

#define MAX_CLIENTS 64
#define BUFFER_SIZE 1024

std::string buildTrailing(const std::vector<std::string>& params, size_t start);

class Server {
    private:
        int                         _serverFd;
        std::string                 _password;
        std::vector<struct pollfd>  _fds;
        std::vector<Client>         _clients;
        Parser                      _parser;

        typedef void (Server::*CommandHandler)(Client&, std::vector<std::string>&);
        std::map<std::string, CommandHandler> _commands;
        
        void setupSocket(int port);
        void acceptClient();
        void handleClient(int idx);
        void removeClient(int idx);
        void dispatch(Client& client, const std::string& line);
        void sendReply(Client& client, const std::string& msg);

        void cmdPass(Client& client, std::vector<std::string>& params);
        void cmdNick(Client& client, std::vector<std::string>& params);
        void cmdUser(Client& client, std::vector<std::string>& params);
        void cmdQuit(Client& client, std::vector<std::string>& params);
        void cmdPing(Client& client, std::vector<std::string>& params);
        void cmdPong(Client& client, std::vector<std::string>& params);
        void cmdPrivmsg(Client& client, std::vector<std::string>& params);
        void cmdNotice(Client& client, std::vector<std::string>& params);
        void cmdMode(Client& client, std::vector<std::string>& params);
        void cmdJoin(Client& client, std::vector<std::string>& params);
        void cmdPart(Client& client, std::vector<std::string>& params);
        void cmdTopic(Client& client, std::vector<std::string>& params);
        void cmdKick(Client& client, std::vector<std::string>& params);
        void cmdInvite(Client& client, std::vector<std::string>& params);

        void sendToClient(int fd, const std::string& message);
        void cmdCap(Client& client, std::vector<std::string>& params);
        Client* findClientByNick(const std::string& nick);
        Channel* findChannelByName(const std::string& name);

        std::vector<Channel> _channels;

    public:
        Server(int port, const std::string& password);
        ~Server();
        void run();
};
