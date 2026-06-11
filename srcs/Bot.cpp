#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <netdb.h>
#include <cctype>

#define BUFFER_SIZE 1024


static void sendMsg(int fd, const std::string& msg)
{
    std::string full = msg + "\r\n";
    size_t total = full.size();
    size_t sent = 0;
    while (sent < total)
    {
        int bytes = send(fd, full.c_str() + sent, total - sent, 0);
        if (bytes <= 0)
            break;
        sent += bytes;
    }
}



static void handleLine(int fd, const std::string& line)
{
    if (line.substr(0, 4) == "PING")
    {
        std::cout << "[recv] " << line << std::endl;
        std::cout << "[send] PONG :" << line.substr(5) << std::endl;
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
    
    // Convert to lowercase for command comparison
    std::string cmd = message;
    for (size_t i = 0; i < cmd.size(); i++)
        cmd[i] = tolower(cmd[i]);

    // typical commands of bot
    if (cmd == "!hello")
        sendMsg(fd, "PRIVMSG " + replyTo + " :Hello " + sender +" !");
    else if (cmd == "!help")
        sendMsg(fd, "PRIVMSG " + replyTo + " :Commandes : !hello, !help, !echo <texte>");
    else if (cmd.size() > 6 && cmd.substr(0, 6) == "!echo ")
        sendMsg(fd, "PRIVMSG " + replyTo + " :" + message.substr(6));
}



int main(int argc, char** argv)
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <password>" << std::endl;
        return 1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        std::cerr << "socket() failed\n";
        return 1;
    }

    struct addrinfo hints, *result;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(argv[1], argv[2], &hints, &result) != 0)
    {
        std::cerr << "getaddrinfo() failed\n";
        close(fd);
        return 1;
    }

    if (connect(fd, result->ai_addr, result->ai_addrlen) < 0)
    {
        std::cerr << "connect() failed\n";
        freeaddrinfo(result);
        close(fd);
        return 1;
    }
    freeaddrinfo(result);

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

