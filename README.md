*This project has been created as part of the 42 curriculum by edidier, bde-la-p and mamakaro.*

# ft_irc


## Table of Contents

-[Description](#description)
-[Instructions](#instructions)
-[Usage](#usage)
-[Features](#features)
-[Technical Choices](#technical-choices)
-[Project Architecture](#project-architecture)
-[Resources](#resources)

## Description

`ft_irc`is an implementation of an **IRC server** (Internet Relay Chat) in C++ 98 standard. 
The goal is to reproduce the core of the IRC protocol so that standard market clients (irssi, HexChat, WeeChat…) can connect to it, authenticate, exchange private messages, and communicate within channels.

The server accepts multiple simultaneous connections and relies on a **single call to `poll()`** to multiplex all input/output (listening socket + client sockets), without ever blocking on a read or a write. It handles password authentication, full user registration (`PASS` / `NICK` / `USER`), the creation and administration of channels with an operator system and channel modes, as well as the reassembly of fragmented messages split on `\r\n`.

A standalone **IRC bot** (bonus) is also provided: it connects to server like a regular client and responds to a few simple commands. 

**Subject contraints:**
- Language: **C++98** (`-Wall -Wextra -Werror -std=c++98`)
- A single `poll()` (or equivalent) for all I/O operations
- **Non-blocking** sockets
- Communication over **TCP/IP (IPv4)**


