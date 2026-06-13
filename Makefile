# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ttsubo <ttsubo@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/07/13 12:46:57 by ttsubo            #+#    #+#              #
#    Updated: 2026/05/29 20:57:22 by ttsubo           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME	= ircserv
CPP 	= c++
CPP_FLG = -std=c++98 -Wall -Wextra -Werror

SRCS_DIR = srcs
INCS_DIR = incs
OBJS_DIR = objs

SRC 	= main.cpp Server.cpp Client.cpp Commands.cpp Channel.cpp
SRCS	= $(addprefix $(SRCS_DIR)/, $(SRC)) 
OBJS 	= $(SRCS:$(SRCS_DIR)/%.cpp=$(OBJS_DIR)/%.o)

all:$(NAME)

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

$(NAME):$(OBJS)
	$(CPP) $(CPP_FLG) $(OBJS) -o $@

$(OBJS_DIR)/%.o:$(SRCS_DIR)/%.cpp | $(OBJS_DIR)
	$(CPP) $(CPP_FLG) -I $(INCS_DIR) -c $< -o $@

test: all
	bash ./tests/run_tests.sh

clean:
	rm -rf $(OBJS)
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -rf $(NAME)

re: fclean all

.PHONY: all clean fclean re debug
