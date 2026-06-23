#include "Server.hpp"
#include <sstream>
#include <algorithm>

struct ChannelNameEquals {
    std::string name;

    ChannelNameEquals(const std::string& channelName) : name(channelName) {}

    bool operator()(const Channel& channel) const {
        return channel.getName() == name;
    }
};

void Server::cmdMode(Client& client, std::vector<std::string>& params)
{
    if (params.empty())
    {
        sendReply(client, ":ircserv 461 " + client.getNickname() + " MODE :Not enough parameters");
        return;
    }

    std::string target = params[0];
    if (target[0] != '#')
        return;
    
    Channel *chan = findChannelByName(target);
    if (!chan)
    {
        sendReply(client, ":ircserv 403 " + client.getNickname() + " " + target + " :No such channel");
        return;
    }

    if (params.size() == 1)
    {
        std::string modeStr = "+";
        if (chan->isInviteOnly())
            modeStr += "i";
        if (chan->isTopicRestricted())
            modeStr += "t";
        if (chan->isKeyEnabled())
            modeStr += "k";
        if (chan->isUserLimitEnabled())
            modeStr += "l";
        sendReply(client, ":ircserv 324 " + client.getNickname() + " " + target + " " + modeStr);
        return;
    }

    if (!chan->isOperator(client.getFd()))
    {
        sendReply(client, ":ircserv 482 " + client.getNickname() + " " + target + " :You're not channel operator");
        return;
    }

    std::string modeStr = params[1];
    size_t paramIdx = 2;
    char sign = '+';

    for (size_t i = 0; i < modeStr.size(); i++)
    {
        char m = modeStr[i];
        if (m == '+'|| m == '-')
        {
            sign = m;
            continue;
        }
        if (m == 'i')
            chan->setInviteOnly(sign == '+');
        else if (m == 't')
            chan->setTopicRestricted(sign == '+');
        else if (m == 'k')
        {
            if (sign == '+')
            {
                if (paramIdx >= params.size())
                {
                    sendReply(client, ":ircserv 461 " + client.getNickname() + " MODE :Not enough parameters");
                    return;
                }
                chan->setKey(params[paramIdx++]);
            }
            else
                chan-> clearKey();
        }
        else if (m == 'o')
        {
                if (paramIdx < params.size())
                {
                    std::string opNick = params[paramIdx++];
                    Client *op = findClientByNick(opNick);
                    if (!op || !chan->hasMember(op->getFd())) {
                        sendReply(client, ":ircserv 441 " + client.getNickname() + " " + opNick + " " + target + " :They aren't on that channel");
                        continue;
                    }
                    if (sign == '+')
                        chan->addOperator(op->getFd());
                    else
                        chan->removeOperator(op->getFd());
                   }
        }
        else if (m == 'l')
        {
            if (sign == '+')
            {
                if (paramIdx >= params.size()) {
                    sendReply(client, ":ircserv 461 " + client.getNickname() + " MODE :Not enough parameters");
                    return;
                }
                std::istringstream iss(params[paramIdx++]);
                size_t limit;
                iss >> limit;
                chan->setUserLimit(limit);
            }
            else
                chan->clearUserLimit();
        }
        else if (m == 'n')
            chan->setNoOutsideMessages(sign == '+');
        else
            sendReply(client, ":ircserv 472 " + client.getNickname() + " " + std::string(1, m) + " :is unknown mode char to me");
    }
    std::string echo = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost MODE " + target;
    for (size_t i = 1; i < params.size(); i++)
        echo += " " + params[i];
    broadcastToChannel(*chan, echo);
}


void Server::cmdJoin(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered()) {
        sendReply(client, ":ircserv 451 * :PASS is not validated yet");
        return;
    }
    if (params.empty()) {
        sendReply(client, ":ircserv 461 JOIN :Not enough parameters");
        return;
    }
    std::string channelName = params[0];
    if (channelName.empty() || channelName[0] != '#') {
        sendReply(client, ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel");        
        return;
    }
    
    std::string key = (params.size() > 1) ? params[1] : "";
    
    Channel* chan = findChannelByName(channelName);
    if (!chan)
    {
        _channels.push_back(Channel(channelName));
        chan = &_channels.back();
    }

    if (chan->isKeyEnabled()) {
        if (key.empty() || key != chan->getKey()) {
            sendReply(client, ":ircserv 475 " + client.getNickname() + " " + channelName + " :Cannot join channel (bad key)");
            return;
        }
    }

    if (!chan->canJoin(client.getFd())) {
        if (chan->isInviteOnly())
            sendReply(client, ":ircserv 473 " + client.getNickname() + " " + channelName + " :Cannot join channel (invite only)");
        else if (chan->isUserLimitEnabled())
            sendReply(client, ":ircserv 471 " + client.getNickname() + " " + channelName + " :Cannot join channel (channel is full)");
        else
            sendReply(client, ":ircserv 471 " + client.getNickname() + " " + channelName + " :Cannot join channel");
        return;
    }

    if (!chan->addMember(client.getFd())) {
        sendReply(client, ":ircserv 443 " + client.getNickname() + " " + channelName + " :is already on channel");
        return;
    }

    std::string joinMsg = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost JOIN " + channelName;
    broadcastToChannel(*chan, joinMsg);

    if (!chan->getTopic().empty())
        sendReply(client, ":ircserv 332 " + client.getNickname() + " " + channelName + " :" + chan->getTopic());
    else
        sendReply(client, ":ircserv 331 " + client.getNickname() + " " + channelName + " :No topic is set");
}

void Server::cmdPart(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered())
    {
        sendReply(client, ":ircserv 451 :You have not registered");
        return ;
    }
    if (params.empty())
    {
        sendReply(client, ":ircserv 461 PART :Not enough parameters");
        return ;
    }
    std::string channelName = params[0];
    Channel* chan = findChannelByName(channelName);
    if (!chan)
    {
        sendReply(client, ":ircserv 403 " + channelName + " :No such channel");
        return ;
    }
    if (!chan->hasMember(client.getFd()))
    {
        sendReply(client, ":ircserv 442 " + channelName + " :You're not on that channel");
        return ;
    }
    std::string message = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost PART " + channelName;
    broadcastToChannel(*chan, message);
    chan->removeMember(client.getFd());

    if (chan->getMemberCount() > 0)
    {
        std::vector<int> members = chan->getMembers();
        bool hasOp = false;
        for (size_t i = 0; i < members.size(); i++)
        {
            if (chan->isOperator(members[i]))
            {
                hasOp = true;
                break;
            }
        }
        if (!hasOp)
            chan->addOperator(members[0]);
    }

    if (chan->getMemberCount() == 0)
    {
        std::string channelNameToErase = chan->getName();
        _channels.erase(std::remove_if(_channels.begin(), _channels.end(), ChannelNameEquals(channelNameToErase)), _channels.end());
    }
}

void Server::cmdTopic(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered()) {
        sendReply(client, ":ircserv 451 :You have not registered");
        return;
    }
    if (params.empty()) {
        sendReply(client, ":ircserv 461 TOPIC :Not enough parameters");
        return;
    }
    std::string channelName = params[0];
    Channel* chan = findChannelByName(channelName);
    if (!chan) {
        sendReply(client, ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel");
        return;
    }
    if (params.size() == 1) {
        if (chan->getTopic().empty())
            sendReply(client, ":ircserv 331 " + client.getNickname() + " " + channelName + " :No topic is set");
        else
            sendReply(client, ":ircserv 332 " + client.getNickname() + " " + channelName + " :" + chan->getTopic());
        return;
    }

    if (!chan->hasMember(client.getFd())) {
        sendReply(client, ":ircserv 442 " + client.getNickname() + " " + channelName + " :You're not on that channel");
        return;
    }

    if (chan->isTopicRestricted() && !chan->isOperator(client.getFd())) {
        sendReply(client, ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not channel operator");
        return;
    }

    std::string newTopic = buildTrailing(params, 1);
    chan->setTopic(newTopic);
    std::string echo = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost TOPIC " + channelName + " :" + newTopic;
    broadcastToChannel(*chan, echo);
}

void Server::cmdKick(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered()) {
        sendReply(client, ":ircserv 451 :You have not registered");
        return;
    }
    if (params.size() < 2) {
        sendReply(client, ":ircserv 461 KICK :Not enough parameters");
        return;
    }
    std::string channelName = params[0];
    std::string targetNick = params[1];
    std::string reason = (params.size() > 2) ? buildTrailing(params, 2) : "Kicked";

    Channel* chan = findChannelByName(channelName);
    if (!chan) {
        sendReply(client, ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel");
        return;
    }
    if (!chan->isOperator(client.getFd())) {
        sendReply(client, ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not channel operator");
        return;
    }
    Client* target = findClientByNick(targetNick);
    if (!target) {
        sendReply(client, ":ircserv 441 " + client.getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }
    if (!chan->hasMember(target->getFd())) {
        sendReply(client, ":ircserv 441 " + client.getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel");
        return;
    }
    std::string kickMsg = ":" + client.getNickname() + "!" + client.getUsername() + "@localhost KICK " + channelName + " " + targetNick + " :" + reason;
    broadcastToChannel(*chan, kickMsg);
    chan->removeMember(target->getFd());

    if (chan->getMemberCount() > 0)
    {
        std::vector<int> members = chan->getMembers();
        bool hasOp = false;
        for (size_t i = 0; i < members.size(); i++)
        {
            if (chan->isOperator(members[i]))
            {
                hasOp = true;
                break;
            }
        }
        if (!hasOp)
            chan->addOperator(members[0]);
    }
    if (chan->getMemberCount() == 0) {
        std::string channelNameToErase = chan->getName();
        _channels.erase(std::remove_if(_channels.begin(), _channels.end(), ChannelNameEquals(channelNameToErase)), _channels.end());
    }
}


void Server::cmdInvite(Client& client, std::vector<std::string>& params)
{
    if (!client.isRegistered()) {
        sendReply(client, ":ircserv 451 :You have not registered");
        return;
    }
    if (params.size() < 2) {
        sendReply(client, ":ircserv 461 INVITE :Not enough parameters");
        return;
    }
    std::string targetNick = params[0];
    std::string channelName = params[1];
    Client* target = findClientByNick(targetNick);
    if (!target) {
        sendReply(client, ":ircserv 401 " + client.getNickname() + " " + targetNick + " :No such nick");
        return;
    }
    Channel* chan = findChannelByName(channelName);
    if (!chan) {
        sendReply(client, ":ircserv 403 " + client.getNickname() + " " + channelName + " :No such channel");
        return;
    }
    if (!chan->hasMember(client.getFd())) {
        sendReply(client, ":ircserv 442 " + client.getNickname() + " " + channelName + " :You're not on that channel");
        return;
    }
    if (!chan->isOperator(client.getFd())) {
        sendReply(client, ":ircserv 482 " + client.getNickname() + " " + channelName + " :You're not channel operator");
        return;
    }
    chan->invite(target->getFd());
    sendReply(*target, ":" + client.getNickname() + "!" + client.getUsername() + "@localhost INVITE " + targetNick + " :" + channelName);
    sendReply(client, ":ircserv 341 " + client.getNickname() + " " + targetNick + " " + channelName);
}