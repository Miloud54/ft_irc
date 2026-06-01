#include "Server.hpp"
#include <sys/socket.h>
#include <sstream>

std::string buildTrailing(const std::vector<std::string>& params, size_t start) {
    std::string msg;
    for (size_t i = start; i < params.size(); i++) {
        if (i > start) msg += " ";
        msg += params[i];
    }
    if (!msg.empty() && msg[0] == ':')
        msg = msg.substr(1);
    return msg;
}


static std::vector<std::string> splitline(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    while (iss >> token)
        tokens.push_back(token);
    return tokens;
}

void Server::sendToClient(int fd, const std::string& message) {
    send(fd, message.c_str(), message.size(), 0);
}

Client* Server::findClientByNick(const std::string& nick) {
    for (size_t i = 0; i < _clients.size(); i++) {
        if (_clients[i].getNickname() == nick)
            return &_clients[i];
    }
    return nullptr;
}

Channel* Server::findChannelByName(const std::string& name) {
    std::map<std::string, Channel*>::iterator it = _nameToChannel.find(name);
    if (it != _nameToChannel.end())
        return it->second;
    return nullptr;
}



