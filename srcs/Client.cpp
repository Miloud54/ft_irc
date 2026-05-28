/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: edidier <edidier@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/28 11:50:52 by edidier           #+#    #+#             */
/*   Updated: 2026/05/28 14:11:14 by edidier          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include <string>

Client::Client(int fd) : _fd(fd), _registered(false), _nickname(""), _username(""), _realname("") {}

Client::~Client() {
    close(_fd);
}

int Client::getFd() {
    return _fd;
}

bool Client::isRegistered() {
    return _registered;
}

std::string Client::getNickname () {
    return _nickname;
}

std::string Client::getUsername() {
    return _username;
}

std::string Client::getRealname() {
    return _realname;
}

std::string Client::getBuffer() {
    return _readBuffer;
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

void Client::setRegistered(bool val) {
    _registered = val;
}

void Client::appendToBuffer(std::string& data) {
    _readBuffer += data;
}

std::string Client::extractLine() {
    size_t pos = _readBuffer.find("\r\n");
    if (pos == std::string::npos)
        return "";
    std::string line = _readBuffer.substr(0, pos);
    _readBuffer = _readBuffer.substr(pos + 2);
    return line;
}