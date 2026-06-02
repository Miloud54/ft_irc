#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024


static void sendMsg(int fd, const std::string& msg)
{
    std::string full = msg + "\r\n";
    send(fd, full.c_str(), full.size(), 0);
}



static void handleLine(int fd, const std::string& line)
{
    if (line.substr(0, 4) == "PING")
    {
        sendMsg(fd, "PONG :" + line.substr(5));
        return;
    }

    // Treat only PRIVMSG
    if (line.find("PRIVMSG") == std::string::npos)
        return;
    
    // extrude the name of the sender( ex: :bob!user@host PRIVMSG #chan :!hello)
    std::string sender;
    if (line[0] == ':')
    {
        size_t space = line.find(' ');
        size_t excl = line.find('!');

        // format :nick!user@host
        if (excl != std::string::npos && excl < space)
            sender = line.substr(1, excl - 1);
        // format :nick (sans !user@host)
        else if (space != std::string::npos)
            sender = line.substr(1, space - 1);
    }

    size_t pos = line.find("PRIVMSG ");
    if (pos == std::string::npos)
        return;
    std::string rest = line.substr(pos + 8);

    size_t sp = rest.find(' ');
    if (sp == std::string::npos)
        return;

    std::string target = rest.substr(0, sp);
    std::string message = rest.substr(sp + 1);
    if (!message.empty() && message[0] == ':')
        message = message.substr(1);
    
    std::string replyTo;

    // If the message is send in the channel, the bot responds in the channel
    // if its a private message, the target is the bot himself
    if (target[0] == '#')
        replyTo = target;
    else
        replyTo = sender;
    
    // typical commands of bot
    if (message == "!hello")
        sendMsg(fd, "PRIVMSG " + replyTo + " :Hello " + sender +" !");
    else if (message == "!help")
        sendMsg(fd, "PRIVMSG " + replyTo + " :Commandes : !hello, !help, !echo <texte>");
    else if (message.size() > 6 && message.substr(0, 6) == "!echo ")
        sendMsg(fd, "PRIVMSG " + replyTo + " :" + message.substr(6));
}



int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <password>" << std::endl;
        return 1;
    }

    int port = std::atoi(argv[2]);
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        std::cerr << "socket() failed\n";
        return 1;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "connect() failed\n";
        return 1;
    }

    sendMsg(fd, "PASS " + std::string(argv[3]));
    sendMsg(fd, "NICK IRCbot");
    sendMsg(fd, "USER ircbot 0 * :IRC Bot");
    sendMsg(fd, "JOIN #general");

    char buf[BUFFER_SIZE];
    std::string pending;

    // read the server
    while (true)
    {
        std::memset(buf, 0, sizeof(buf));
        int bytes = recv(fd, buf, sizeof(buf) - 1, 0);
        if (bytes <= 0)
        {
            std::cout << "Disconnected\n";
            break;
        }
        // recompose the message received by recv
        pending.append(buf, bytes);

        size_t pos;
        while ((pos = pending.find("\r\n")) != std::string::npos)
        {
            std::string line = pending.substr(0, pos);
            pending.erase(0, pos + 2);
            std::cout << "[recv] " << line << "\n";
            handleLine(fd, line);
        }
    }
    close(fd);
    return 0;
}

