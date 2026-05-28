#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <map>


struct IrcCommand
{
    std::string prefix;
    std::string command;
    std::vector<std::string> params;
};


class Parser
{
    private:
        std::map<int, std::string> _buffers;
};






#endif