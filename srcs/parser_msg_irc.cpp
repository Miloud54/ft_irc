

#include <sstream>
#include "parser_msg_irc.hpp"

Parser::Parser()
{

}

Parser::~Parser()
{

}

void Parser::removeClient(int clientFd)
{
    _buffers.erase(clientFd);
}

IrcCommand Parser::parseLine(const std::string& line)
{
    IrcCommand cmd;
    std::istringstream iss(line);
    std::string token;

    // Check for prefix (starts with :)
    if (line[0] == ':')
    {
        iss >> token;
        cmd.prefix = token.substr(1);
        iss >> cmd.command;
    }
    else
    {
        iss >> cmd.command;
    }

    while (iss >> token)
    {
        if (token[0] == ':')
        {
            std::string trailing = token.substr(1);
            std::string rest;
            if (std::getline(iss, rest))
                trailing += rest;
            cmd.params.push_back(trailing);
            break;
        }
        else
        {
            cmd.params.push_back(token);
        }
    }
    return cmd;


}

 std::vector<IrcCommand> Parser::feed(int clientFd, const char *data, int bytes)
 {
    std::vector<IrcCommand> commands;

    if (bytes <= 0)
        return commands;
    
    // Add data to the buffer to this client
    _buffers[clientFd].append(data, bytes);

    // Parse all complete lines (ending with \r \n)
    size_t pos = 0;
    while ((pos = _buffers[clientFd].find("\r\n")) != std::string::npos)
    {
        // Extract the complete line
        std::string line = _buffers[clientFd].substr(0, pos);

        // Remove \r\n from buffer
        _buffers[clientFd].erase(0, pos + 2);

        // Parse the line and add to commands
        if (!line.empty())
            commands.push_back(parseLine(line));
    }
    return commands;    
 }




/*
"NICK maria\r\n" → command: NICK, params: [maria]
"JOIN #general\r\n" → command: JOIN, params: [#general]
"PRIVMSG #general :salut tout le monde\r\n" → command: PRIVMSG, params: [#general], trailing: "salut tout le monde"
*/



