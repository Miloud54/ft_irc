*This project has been created as part of the 42 curriculum by edidier, bde-la-p and mamakaro.*
 
# ft_irc
 
> An **IRC** server written in C++98, protocol-compliant, able to handle multiple clients simultaneously through a single event loop (`poll`).
 
---
 
## Table of Contents
 
- [Description](#description)
- [Instructions](#instructions)
- [Usage](#usage)
- [Features](#features)
- [Technical Choices](#technical-choices)
- [Project Architecture](#project-architecture)
- [Resources](#resources)
---
 
## Description
 
`ft_irc` is an implementation of an **IRC server** (Internet Relay Chat) in C++98. The goal is to reproduce the core of the IRC protocol so that standard market clients (irssi, HexChat, WeeChat…) can connect to it, authenticate, exchange private messages, and communicate within channels.
 
The server accepts multiple simultaneous connections and relies on a **single call to `poll()`** to multiplex all input/output (listening socket + client sockets), without ever blocking on a read or a write. It handles password authentication, full user registration (`PASS` / `NICK` / `USER`), the creation and administration of channels with an operator system and channel modes, as well as the reassembly of fragmented messages split on `\r\n`.
 
A standalone **IRC bot** (bonus) is also provided: it connects to the server like a regular client and responds to a few simple commands.
 
**Subject constraints:**
- Language: **C++98** (`-Wall -Wextra -Werror -std=c++98`)
- A single `poll()` (or equivalent) for all I/O operations
- **Non-blocking** sockets
- Communication over **TCP/IP (IPv4)**
---
 
## Instructions
 
### Requirements
 
- A C++ compiler supporting the **C++98** standard (`c++` / `g++` / `clang++`)
- `make`
- A **Unix-like** system (Linux, macOS)
### Compilation
 
A `Makefile` is provided at the root with the usual rules:
 
```bash
# Build the server  ->  produces the « ircserv » executable
make
 
# Build the bot (bonus)  ->  produces the « ircbot » executable
make bot
 
# Cleaning
make clean      # removes object files
make fclean     # removes object files + executables
make re         # rebuilds everything from scratch
```
 
### Execution
 
```bash
./ircserv <port> <password>
```
 
| Argument     | Description                                              |
|--------------|----------------------------------------------------------|
| `port`       | Server listening port (1 – 65535)                        |
| `password`   | Password required for clients to connect                 |
 
**Example:**
 
```bash
./ircserv 6667 mypassword
```
 
---
 
## Usage
 
### Connecting with a real IRC client (irssi)
 
```bash
# Terminal 1: start the server
./ircserv 6667 mypassword
 
# Terminal 2: connect
irssi
/connect 127.0.0.1 6667 mypassword
/join #general
/msg #general Hello world!
```
 
### Testing manually with netcat
 
```bash
nc 127.0.0.1 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice
JOIN #general
PRIVMSG #general :Hello !
```

### File transfer

- Send
```bash
/dcc send bob /tmp/file.txt
```
- Accept
```bash
/dcc get alice
```

### Running the bot (bonus)
 
```bash
./ircbot <host> <port> <password>
# example
./ircbot 127.0.0.1 6667 mypassword
```
 
The bot joins `#general` and responds to: `!hello`, `!help`, `!echo <text>`. It replies in the channel if the command is sent there, or privately if it is addressed directly to the bot.
 
---
 
## Features
 
### Supported IRC commands
 
| Command    | Purpose                                                              |
|------------|----------------------------------------------------------------------|
| `PASS`     | Provides the server connection password                              |
| `NICK`     | Sets or changes the nickname                                         |
| `USER`     | Registers the user (username, realname)                              |
| `QUIT`     | Disconnects the client from the server                               |
| `PING` / `PONG` | Connection keep-alive mechanism                                 |
| `JOIN`     | Joins (or creates) a channel                                         |
| `PART`     | Leaves a channel                                                     |
| `PRIVMSG`  | Sends a message to a user or a channel                               |
| `NOTICE`   | Sends a notification (no automatic reply)                            |
| `TOPIC`    | Displays or changes a channel's topic                                |
| `MODE`     | Modifies a channel's modes                                           |
| `KICK`     | Ejects a user from a channel                                         |
| `INVITE`   | Invites a user to a channel                                          |
 
### Channel modes (`MODE`)
 
| Mode | Description                                                          |
|------|---------------------------------------------------------------------|
| `+i` / `-i` | **Invite-only** channel                                       |
| `+t` / `-t` | Restricts **topic** changes to operators                      |
| `+k` / `-k` | Sets / removes a channel **key** (password)                   |
| `+o` / `-o` | Grants / revokes **operator** status to a member              |
| `+l` / `-l` | Sets / removes a **user limit** on the channel                |
 
```text
MODE #general +i
MODE #general +k secret
MODE #general +o alice
MODE #general +l 20
```
 
---
 
## Technical Choices
 
- **Multiplexing with `poll()`**: a single event loop watches the listening socket and all client sockets, in line with the subject constraint (a single `poll`).
- **Non-blocking sockets**: no read/write operation blocks the main loop.
- **Command dispatch table**: commands are routed through a `std::map<std::string, CommandHandler>` (pointers to member methods), which makes adding commands simple and readable.
- **Per-client buffer**: each `Client` accumulates incoming data and only extracts a command once a complete line terminated by `\r\n` is available — this handles messages fragmented by TCP.
- **Separation of concerns**: `Server` (networking + dispatch), `Client` (state/registration), `Channel` (members, operators, modes), `Parser` (IRC message parsing).
---
 
## Project Architecture
 
```
ft_irc/
├── Makefile
├── incs/
│   ├── Server.hpp           # Main loop, poll(), command dispatch
│   ├── Signal.hpp           
│   ├── Client.hpp           # Client state (fd, registration, buffers…)
│   ├── Channel.hpp          # Channel: members, operators, modes, topic
│   ├── Bot.hpp              # IRC bot (bonus)
└── srcs/
    ├── main.cpp             # Entry point, argument validation
    ├── Server.cpp           # Server and command implementation
    ├── Client.cpp           # Client state management
    ├── Channel.cpp          # Channel logic
    ├── cmd_helpers.cpp      # Command helper functions
    └── Bot.cpp             # IRC bot (bonus)
```
 
---
 
## Resources
 
### IRC protocol references

- **RFC 1459** — *Internet Relay Chat Protocol*: https://datatracker.ietf.org/doc/html/rfc1459
- **RFC 2812** — *Internet Relay Chat: Client Protocol*: https://datatracker.ietf.org/doc/html/rfc2812

### Network programming / sockets

- **Beej's Guide to Network Programming**: https://beej.us/guide/bgnet/
- `man` pages: `socket(2)`, `bind(2)`, `listen(2)`, `accept(2)`, `poll(2)`, `send(2)`, `recv(2)`, `fcntl(2)`


### Use of AI

- **Understanding the protocol**: use of an AI (e.g. ChatGPT / Claude) to explain certain sections of RFC 1459/2812 and clarify the exact format of the server's numeric replies.
- **Debugging**: help interpreting compilation error messages and analyzing unexpected behavior (e.g. buffer handling, `\r\n` splitting).
- **Documentation**: writing and structuring this README.
- *(Specify what was written/designed by yourselves, as opposed to what AI was used for as assistance.)*
