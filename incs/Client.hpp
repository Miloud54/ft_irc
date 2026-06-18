#pragma once
#include <string>

class Client {
    private:
        int _fd;
        bool _registered;
        bool _passOk;
        bool _nickOk;
        bool _userOk;
        std::string _nickname;
        std::string _username;
        std::string _realname;
        std::string _readBuffer;
        std::string _writeBuffer;
    
    public:
        Client (int fd);
        ~Client();

        int getFd() const;
        bool isRegistered() const;
        bool isPassOk() const;
        bool isNickOk() const;
        bool isUserOk() const;
        std::string getNickname() const;
        std::string getUsername() const;
        std::string getRealname() const;
        std::string getBuffer() const;
        const std::string& getWriteBuffer() const;

        void setRegistered(bool val);
        void setPassOk(bool val);
        void setNickOk(bool val);
        void setUserOk(bool val);
        void setNickname(const std::string& nick);
        void setUsername(const std::string& user);
        void setRealname(const std::string& real);

        void appendToBuffer(const std::string& data);
        void appendToWriteBuffer(const std::string& data);
        void consumeWriteBuffer(size_t n);
        bool hasPendingWrite() const;
        std::string extractLine();        
};