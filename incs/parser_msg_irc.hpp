#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <map>



/*
recv()
  ↓
feed(clientFd, data, bytes)
  ↓
"NICK maria"
  ↓
parseLine("NICK maria")
  ↓
IrcCommand { command = "NICK", params = ["maria"] }
*/


// Parsed IRC command
struct IrcCommand
{
    std::string prefix;
    std::string command;
    std::vector<std::string> params;
};

// Transdorm data received by recv() to IRC cmd
class Parser
{
    private:
        std::map<int, std::string> _buffers;
    IrcCommand parseLine(const std::string& line);


    public:
        Parser();
        ~Parser();
        // recv() can receive a command in multiple parts, feed manages it
        std::vector<IrcCommand> feed(int clientFd, const char *data, int bytes);
        void removeClient(int clientFd);
};



#endif