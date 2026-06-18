#include "Client.hpp"
#include <string>
#include <unistd.h>

Client::Client(int fd) : _fd(fd), _registered(false), _passOk(false), _nickOk(false), _userOk(false), _nickname(""), _username(""), _realname("") {}

Client::~Client() {}

int Client::getFd() const {
    return _fd;
}

bool Client::isRegistered() const {
    return _registered;
}

bool Client::isPassOk() const {
    return _passOk;
}

bool Client::isNickOk() const {
    return _nickOk;
}

bool Client::isUserOk() const {
    return _userOk;
}

std::string Client::getNickname() const {
    return _nickname;
}

std::string Client::getUsername() const {
    return _username;
}

std::string Client::getRealname() const {
    return _realname;
}

std::string Client::getBuffer() const {
    return _readBuffer;
}

const std::string& Client::getWriteBuffer() const {
    return _writeBuffer;
}

void Client::setRegistered(bool val) {
    _registered = val;
}

void Client::setPassOk(bool val) {
    _passOk = val;
}

void Client::setNickOk(bool val) {
    _nickOk = val;
}

void Client::setUserOk(bool val) {
    _userOk = val;
}

void Client::setNickname(const std::string& nick) {
    _nickname = nick;
}

void Client::setUsername(const std::string& user) {
    _username = user;
}

void Client::setRealname(const std::string& real) {
    _realname = real;
}

void Client::appendToBuffer(const std::string& data) {
    _readBuffer += data;
}

void Client::appendToWriteBuffer(const std::string& data) {
    _writeBuffer += data;
}

void Client::consumeWriteBuffer(size_t n) {
    _writeBuffer.erase(0, n);
}

bool Client::hasPendingWrite() const {
    return !_writeBuffer.empty();
}

std::string Client::extractLine() {
    size_t pos = _readBuffer.find("\r\n");
    size_t delimiterSize = 2;

    if (pos == std::string::npos)
    {
        pos = _readBuffer.find('\n');
        delimiterSize = 1;
    }

    if (pos == std::string::npos)
        return "";

    if (delimiterSize == 2 && pos > 0 && _readBuffer[pos - 1] == '\r')
        pos--;

    std::string line = _readBuffer.substr(0, pos);
    _readBuffer = _readBuffer.substr(pos + delimiterSize);
    return line;
}