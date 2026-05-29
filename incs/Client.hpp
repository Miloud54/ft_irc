/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: edidier <edidier@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/28 10:58:40 by edidier           #+#    #+#             */
/*   Updated: 2026/05/29 14:04:46 by edidier          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

        void setRegistered(bool val);
        void setPassOk(bool val);
        void setNickOk(bool val);
        void setUserOk(bool val);
        void setNickname(const std::string& nick);
        void setUsername(const std::string& user);
        void setRealname(const std::string& real);

        void appendToBuffer(const std::string& data);
        std::string extractLine();
};