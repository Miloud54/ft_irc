#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

class Channel {
    private:
        std::string _name;
        std::string _topic;
        std::string _key;
        std::size_t _userLimit;
        bool _inviteOnly;
        bool _topicRestricted;
        bool _keyEnabled;
        bool _userLimitEnabled;
        bool _noOutsideMessages;
        std::set<int> _members;
        std::set<int> _operators;
        std::set<int> _invited;

    public:
        Channel();
        explicit Channel(const std::string& name);
        ~Channel();

        const std::string& getName() const;
        const std::string& getTopic() const;
        const std::string& getKey() const;
        std::size_t getUserLimit() const;
        bool isInviteOnly() const;
        bool isTopicRestricted() const;
        bool isKeyEnabled() const;
        bool isUserLimitEnabled() const;

        std::size_t getMemberCount() const;
        bool hasMember(int fd) const;
        bool isOperator(int fd) const;
        bool isInvited(int fd) const;

        void setName(const std::string& name);
        void setTopic(const std::string& topic);
        void setKey(const std::string& key);
        void clearKey();
        void setUserLimit(std::size_t limit);
        void clearUserLimit();
        void setInviteOnly(bool enabled);
        void setTopicRestricted(bool enabled);

        bool canJoin(int fd) const;
        bool addMember(int fd);
        void removeMember(int fd);
        void addOperator(int fd);
        void removeOperator(int fd);
        void invite(int fd);
        void revokeInvite(int fd);
        bool isNoOutsideMessages() const;
        void setNoOutsideMessages(bool enabled);

        std::vector<int> getMembers() const;
        std::vector<int> getNonOperatorMembers() const;
};

#endif