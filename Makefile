# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: mamakaro <mamakaro@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/05/27 18:33:08 by edidier           #+#    #+#              #
#    Updated: 2026/05/28 17:53:14 by mamakaro         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		=	ircserv
CXX			=	c++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++98 -Iincs

SRCS	= srcs/main.cpp srcs/Server.cpp srcs/Client.cpp srcs/Channel.cpp srcs/parser_msg_irc.cpp srcs/cmd_helpers.cpp
OBJS	= $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
