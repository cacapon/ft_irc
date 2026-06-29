# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ttsubo <ttsubo@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/07/13 12:46:57 by ttsubo            #+#    #+#              #
#    Updated: 2026/06/29 21:57:18 by ttsubo           ###   ########.fr        #
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

UNIT_TEST_DIR	= tests/unit
UNIT_TEST_BIN	= unit_tests
UNIT_TEST_SRCS	= $(UNIT_TEST_DIR)/test_main.cpp $(UNIT_TEST_DIR)/test_parse.cpp $(UNIT_TEST_DIR)/test_replies.cpp
UNIT_SRCS		= $(SRCS_DIR)/Commands.cpp $(SRCS_DIR)/Client.cpp $(SRCS_DIR)/Channel.cpp $(SRCS_DIR)/Server.cpp

unit_test:
	$(CPP) $(CPP_FLG) -I $(INCS_DIR) -I $(UNIT_TEST_DIR) $(UNIT_TEST_SRCS) $(UNIT_SRCS) -o $(UNIT_TEST_BIN)
	./$(UNIT_TEST_BIN)

clean:
	rm -rf $(OBJS)
	rm -rf $(OBJS_DIR)
	rm -f $(UNIT_TEST_BIN)

fclean: clean
	rm -rf $(NAME)

re: fclean all

setup-hooks:
	git config core.hooksPath .githooks

.PHONY: all clean fclean re test unit_test setup-hooks
