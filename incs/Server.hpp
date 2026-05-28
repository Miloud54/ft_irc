/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: edidier <edidier@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 16:45:50 by edidier           #+#    #+#             */
/*   Updated: 2026/05/27 18:34:13 by edidier          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <vector>
#include <poll.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

class Server {
    public:
        Server(int port);
        ~Server();
        void run();

    private:
        int serverFd;
        std::vector<struct pollfd> fds;

        void setupSocket(int port);
        void acceptClient();
        void handleClient(int idx);
        void removeClient(int idx);
};