/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: edidier <edidier@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 16:45:50 by edidier           #+#    #+#             */
/*   Updated: 2026/05/29 14:13:44 by edidier          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <vector>
#include <map>
#include <string>
#include <poll.h>
#include "Client.hpp"

#define MAX_CLIENTS 64
#define BUFFER_SIZE 1024

class Server {
    private:
        int                         _serverFd;
        std::string                 _password;
        std::vector<struct pollfd>  _fds;
        std::vector<Client>         _clients;

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

    public:
        Server(int port, const std::string& password);
        ~Server();
        void run();
};