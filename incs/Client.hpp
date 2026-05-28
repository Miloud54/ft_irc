/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: edidier <edidier@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/05/28 10:58:40 by edidier           #+#    #+#             */
/*   Updated: 2026/05/28 17:39:57 by edidier          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once
#include <string>

class Client {
    private:
        int _fd;
        bool _registered;
        std::string _nickname;
        std::string _username;
        std::string _realname;
        std::string _readBuffer;
    
    public:
        Client (int fd);
        ~Client();

        int getFd() const;
        bool isRegistered() const;
        std::string getNickname() const;
        std::string getUsername() const;
        std::string getRealname() const;
        std::string getBuffer() const;

        void setNickname(const std::string& nick);
        void setUsername(const std::string& user);
        void setRealname(const std::string& real);
        void setRegistered(bool val);

        void appendToBuffer(const std::string& data);
        std::string extractLine();
};