/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mamakaro <mamakaro@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/26 11:07:46 by edidier           #+#    #+#             */
/*   Updated: 2026/05/28 12:34:22 by mamakaro         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "inc/server.hpp"
#include <iostream>
#include <cstdlib>

int main(int ac, char **av)
{
    if (ac != 2) 
    {   
        std::cerr << "Usage: ./echo_server <port>" << std::endl;
        return 1;
    }

    int port = atoi(av[1]);

    if (port <= 0 || port > 65535)
    {
        std::cerr << "Invalid port" << std::endl;
        return 1;
    }

    try 
    {
        Server server(port);
        server.run();
    } 
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
