/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: edidier <edidier@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/22 16:13:02 by edidier           #+#    #+#             */
/*   Updated: 2026/05/28 10:58:31 by edidier          ###   ########.fr       */
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

Server::Server(int port) {
    fds.reserve(64);
    setupSocket(port);
}

Server::~Server() {
    for (size_t i = 0; i < fds.size(); i++)
        close(fds[i].fd);
}

void Server::setupSocket(int port) {
    /*Cree socket TCP (SOCK_STREAM = flux fiable, AF_INET = IPv4)*/
    /*Retourne un Fd (tout est fichier sous Unix)*/
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
        throw std::runtime_error("socket() failed"); 

    /*SO_REUSADDR permet de relancer serveur immediatement apres un arret
    sans attendre expiration du TIME_WAIT (env 60s)*/
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");

    /*Mode non-bloquant : les appels comme accept() et recv() retournent immediatement
    avec -1 (EAGAIN) s'ils n'ont rien a faire au lieu de bloquer le programme*/
    fcntl(serverFd, F_SETFL, O_NONBLOCK);

    /*Config l'addresse d'ecoute*/
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        throw std::runtime_error("bind() failed");

    if (listen(serverFd, 10) < 0)
        throw std::runtime_error("listen() failed");

    
    struct pollfd pfd;
    pfd.fd = serverFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    fds.push_back(pfd);

    std::cout << "Listenning to port " << port << std::endl;
}

void Server::run() {
    while (true)
    {
        /*poll() surveille tous les Fds et bloque jusqu'a ce qu'au moins un soit pret. 
        Timout -1 = attend idefiniment.
        C'est le coeur du serveur : une seule fonction gere tous les clients*/
        int ready = poll(fds.data(), fds.size(), -1);
        if (ready < 0)
            throw std::runtime_error("poll() failed");
        
        /*Parcours tous les Fds pour trouver ceux qui sont prets*/
        for (int i = 0; i < fds.size(); i++)
        {
            /*revents est rempli par poll() - POLLIN = donnees dispo
            Si ce Fd n'a rien a lire, on passe au suivant*/
            if (!(fds[i].revents & POLLIN))
                continue;

            if (fds[i].fd == serverFd)
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
    int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientLen);
    if (clientFd < 0) 
        throw std::runtime_error("accept() failed");

    /*Mode non-bloquant*/
    fcntl(clientFd, F_SETFL, O_NONBLOCK);

    /*Ajoute nouveau client a la liste surveillee par poll()*/
    struct pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;
    fds.push_back(pfd);

    std::cout << "New client connected (fd=" << clientFd << ")" << std::endl;
}

void Server::handleClient(int idx) {
    char buf[BUFFER_SIZE];
    memset(buf, 0, sizeof(buf));
  
    /*revc() lit les donnee dispo sur ce Fd
    Retourne : nb d'octets lus, 0 si deco propre, -1 si erreur*/
    int bytes = recv(fds[idx].fd, buf, sizeof(buf) - 1, 0);

    if (bytes <= 0)
    {
        /*0 = deco propre (client a ferme la connexion)
        <0 = erreur reseau (connexion interrompue brutalement)*/
        if (bytes == 0)
            std::cout << "Client disconnected (fd=" << fds[idx].fd << ")" << std::endl;
        else
            std::cerr << "recv( error on fd=" << fds[idx].fd << " )" << std::endl;
        removeClient(idx);
        return;
    }
    if (bytes > 0)
    {
        std::cout << "Received: " << buf;
        
        /*Echo : renvoie exactement ce qu'on a recu
        send() peut envoyer moins que demande - acceptable pour un echo server => a boucler pour IRC*/
        send(fds[idx].fd, buf, bytes, 0);
    }
}

void Server::removeClient(int idx) {
    /*Ferme Fd et libere ressource syst*/
    close(fds[idx].fd);

    /* Supprime du vecteur en remplacant par le dernier element (plus efficace 
    qu'un erase() qui decale tous les elements)
    A modif pour IRC : 
    fds[idx] = fds.back();
    fds.pop_back();*/
    fds.erase(fds.begin() + idx);
}