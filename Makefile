# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: mamakaro <mamakaro@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/05/27 18:33:08 by edidier           #+#    #+#              #
#    Updated: 2026/05/28 12:52:00 by mamakaro         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME	=	echo_server
CXX		=	c++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++98 -Iinc	

SRCS	= srcs/main.cpp srcs/server.cpp
OBJS	= $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -rf $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
