#include "Signal.hpp"
#include "../incs/Server.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

int main(int ac, char **av)
{
    if (ac != 3) 
    {   
        std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
        return 1;
    }

    int port = atoi(av[1]);
    std::string password = av[2];

    if (port <= 0 || port > 65535)
    {
        std::cerr << "Invalid port" << std::endl;
        return 1;
    }

    signal(SIGINT, signalHandler);
    
    try 
    {
        Server server(port, password);
        server.run();
    } 
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
