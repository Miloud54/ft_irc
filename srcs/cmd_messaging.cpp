#include "Server.hpp"

void Server::cmdPing(Client& client, std::vector<std::string>& params) {
    if (params.empty()) {
        sendReply(client, ":ircserv 409 " + client.getNickname() + " :No origin specified");
        return;
    }
    std::string token = params[0];

    if (!token.empty() && token[0] == ':')
        token = token.substr(1);
    sendReply(client, "PONG :" + token);
}


void Server::cmdPong(Client& client, std::vector<std::string>& params) {
    (void)params;
    client.setLastPingSent(0);
    client.setLastActivity(std::time(NULL));
}

void Server::cmdPrivmsg(Client& client, std::vector<std::string>& params) {
    if (!client.isRegistered()) {
        sendReply(client, ":ircserv 451 :You have not registered");
        return;
    }
    if (params.empty()) {
        sendReply(client, ":ircserv 411 " + client.getNickname() + " :No recipient given (PRIVMSG)");
        return;
    }
    if (params.size() < 2) {
        sendReply(client, ":ircserv 412 " + client.getNickname() + " :No text to send");
        return;
    }

    std::string target = params[0];
    std::string message = buildTrailing(params, 1);

    if (!message.empty() && message[0] == '\x01') {
        if (target[0] == '#') {
            Channel *chan = findChannelByName(target);
            if (!chan) {
                sendReply(client, ":ircserv 403 " + client.getNickname() + " " + target + " :No such channel");
                return;
            }
            broadcastToChannel(*chan, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PRIVMSG " + target + " :" + message, client.getFd());
        }
        else {
            Client* recipient = findClientByNick(target);
            if (!recipient) {
                sendReply(client, ":ircserv 401 " + client.getNickname() + " " + target + " :No such nick");
                return;
            }
            sendReply(*recipient, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PRIVMSG " + recipient->getNickname() + " :" + message);
        }
        return;
    }

    if (target[0] == '#') {
        Channel *chan = findChannelByName(target);
        if (!chan) {
            sendReply(client, ":ircserv 403 " + client.getNickname() + " " + target + " :No such channel");
            return;
        }
        if (!chan->hasMember(client.getFd())) {
            sendReply(client, ":ircserv 404 " + client.getNickname() + " " + target + " :Cannot send to channel");
            return;
        }
        broadcastToChannel(*chan, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PRIVMSG " + target + " :" + message, client.getFd());    }
    else {
        Client* recipient = findClientByNick(target);
        if (!recipient) {
            sendReply(client, ":ircserv 401 " + client.getNickname() + " " + target + " :No such nick");
            return;
        }
        sendReply(*recipient, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PRIVMSG " + recipient->getNickname() + " :" + message);
    }
}

void Server::cmdNotice(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered() || params.size() < 2)
        return;

    std::string target = params[0];
    std::string message = buildTrailing(params, 1);

    if (!message.empty() && message[0] == '\x01') {
        if (target[0] == '#') {
            Channel* chan = findChannelByName(target);
            if (chan)
                broadcastToChannel(*chan, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost NOTICE " + target + " :" + message, client.getFd());
        } else {
            Client* recipient = findClientByNick(target);
            if (recipient)
                sendReply(*recipient, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost NOTICE " + target + " :" + message);
        }
        return;
    }

    if (target[0] == '#') {
        Channel* chan = findChannelByName(target);
        if (chan)
            broadcastToChannel(*chan, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost NOTICE " + target + " :" + message, client.getFd());
    } else {
        Client* recipient = findClientByNick(target);
        if (recipient)
            sendReply(*recipient, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost NOTICE " + target + " :" + message);
    }
}