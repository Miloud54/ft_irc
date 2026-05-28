# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: edidier <edidier@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/05/27 18:33:08 by edidier           #+#    #+#              #
#    Updated: 2026/05/28 14:54:33 by edidier          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME		=	echo_server
CXX			=	c++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++98 -Iincs

SRCS	= srcs/main.cpp srcs/Server.cpp srcs/Client.cpp
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
