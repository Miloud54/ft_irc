# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: edidier <edidier@student.42.fr>            +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/05/27 18:33:08 by edidier           #+#    #+#              #
#    Updated: 2026/05/27 18:33:25 by edidier          ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME	=	echo_server
CC		=	c++
CFLAGS	=	-Wall -Wextra -Werror -std=c++98	

SRCS	= srcs/main.cpp srcs/server.cpp
OBJS	= $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	@$(CC) $(CFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
