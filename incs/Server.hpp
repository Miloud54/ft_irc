/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: edidier <edidier@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 16:45:50 by edidier           #+#    #+#             */
/*   Updated: 2026/05/28 14:28:32 by edidier          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <vector>
#include <poll.h>
#include "Client.hpp"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

class Server {
    private:
        int _serverFd;
        std::string _password;
        std::vector<struct pollfd> _fds;
        std::vector<Client> _clients;

        void setupSocket(int port);
        void acceptClient();
        void handleClient(int idx);
        void removeClient(int idx);

    public:
        Server(int port, const std::string& password);
        ~Server();
        void run();
};