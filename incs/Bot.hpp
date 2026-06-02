#pragma once
#include <string>

class Bot {
    private:
        int _fd;
        std::string _host;
        int _port;
        std::string _password;
        std::string _pending;

        void sendMsg(const std::string& msg);
        void handleLine(const std::string& line);
    
    public:
        Bot(const std::string& host, int port, const std::string& password);
        ~Bot();
        void connect();
        void run();
};