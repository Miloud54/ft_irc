#include "../incs/Channel.hpp"

Channel::Channel()
    : _name(""), _topic(""), _key(""), _userLimit(0), _inviteOnly(false),
      _topicRestricted(false), _keyEnabled(false), _userLimitEnabled(false) {}

Channel::Channel(const std::string& name)
    : _name(name), _topic(""), _key(""), _userLimit(0), _inviteOnly(false),
      _topicRestricted(false), _keyEnabled(false), _userLimitEnabled(false) {}

Channel::~Channel() {}

const std::string& Channel::getName() const { return _name; }

const std::string& Channel::getTopic() const { return _topic; }

const std::string& Channel::getKey() const { return _key; }

std::size_t Channel::getUserLimit() const { return _userLimit; }

bool Channel::isInviteOnly() const { return _inviteOnly; }

bool Channel::isTopicRestricted() const { return _topicRestricted; }

bool Channel::isKeyEnabled() const { return _keyEnabled; }

bool Channel::isUserLimitEnabled() const { return _userLimitEnabled; }

std::size_t Channel::getMemberCount() const { return _members.size(); }

bool Channel::hasMember(int fd) const { return _members.find(fd) != _members.end(); }

bool Channel::isOperator(int fd) const { return _operators.find(fd) != _operators.end(); }

bool Channel::isInvited(int fd) const { return _invited.find(fd) != _invited.end(); }

void Channel::setName(const std::string& name) { _name = name; }

void Channel::setTopic(const std::string& topic) { _topic = topic; }

void Channel::setKey(const std::string& key) {
    _key = key;
    _keyEnabled = true;
}

void Channel::clearKey() {
    _key.clear();
    _keyEnabled = false;
}

void Channel::setUserLimit(std::size_t limit) {
    _userLimit = limit;
    _userLimitEnabled = true;
}

void Channel::clearUserLimit() {
    _userLimit = 0;
    _userLimitEnabled = false;
}

void Channel::setInviteOnly(bool enabled) { _inviteOnly = enabled; }

void Channel::setTopicRestricted(bool enabled) { _topicRestricted = enabled; }

bool Channel::canJoin(int fd) const {
    if (_userLimitEnabled && _members.size() >= _userLimit)
        return false;
    if (_inviteOnly && !isInvited(fd) && !isOperator(fd))
        return false;
    return true;
}

bool Channel::addMember(int fd) {
    if (!canJoin(fd) || hasMember(fd))
        return false;

    _members.insert(fd);
    if (_members.size() == 1)
        _operators.insert(fd);
    _invited.erase(fd);
    return true;
}

void Channel::removeMember(int fd) {
    _members.erase(fd);
    _operators.erase(fd);
    _invited.erase(fd);
}

void Channel::addOperator(int fd) { _operators.insert(fd); }

void Channel::removeOperator(int fd) { _operators.erase(fd); }

void Channel::invite(int fd) { _invited.insert(fd); }

void Channel::revokeInvite(int fd) { _invited.erase(fd); }

std::vector<int> Channel::getMembers() const {
    std::vector<int> members;
    for (std::set<int>::const_iterator it = _members.begin(); it != _members.end(); ++it)
        members.push_back(*it);
    return members;
}

std::vector<int> Channel::getNonOperatorMembers() const {
    std::vector<int> members;
    for (std::set<int>::const_iterator it = _members.begin(); it != _members.end(); ++it) {
        if (!isOperator(*it))
            members.push_back(*it);
    }
    return members;
}