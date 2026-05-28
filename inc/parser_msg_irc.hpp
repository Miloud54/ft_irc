#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <map>

// Parsed IRC command
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
    IrcCommand parseLine(const std::string& line);


    public:
        Parser();
        ~Parser();

        std::vector<IrcCommand> feed(int clientFd, const char *data, int bytes);
        void removeClient(int clientFd);
};






#endif