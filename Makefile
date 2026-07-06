# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: ttsubo <ttsubo@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/07/13 12:46:57 by ttsubo            #+#    #+#              #
#    Updated: 2026/07/05 10:47:27 by ttsubo           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME	= ircserv
CPP 	= c++
CPP_FLG = -std=c++98 -Wall -Wextra -Werror -pedantic-errors

SRCS_DIR = srcs
INCS_DIR = incs
OBJS_DIR = objs

CMD_SRC = commands/Core.cpp commands/Ping.cpp commands/Pass.cpp commands/Nick.cpp \
          commands/User.cpp commands/Join.cpp commands/Privmsg.cpp commands/Part.cpp \
          commands/Mode.cpp commands/Kick.cpp commands/Invite.cpp commands/Topic.cpp \
          commands/Quit.cpp commands/Guards.cpp
SRC 	= Server.cpp Client.cpp Channel.cpp Utils.cpp $(CMD_SRC)
SRCS	= $(addprefix $(SRCS_DIR)/, $(SRC)) $(addprefix $(SRCS_DIR)/, main.cpp) 
OBJS 	= $(SRCS:$(SRCS_DIR)/%.cpp=$(OBJS_DIR)/%.o)

all:$(NAME)

$(OBJS_DIR):
	mkdir -p $(OBJS_DIR)

$(NAME):$(OBJS)
	$(CPP) $(CPP_FLG) $(OBJS) -o $@

$(OBJS_DIR)/%.o:$(SRCS_DIR)/%.cpp | $(OBJS_DIR)
	@mkdir -p $(dir $@)
	$(CPP) $(CPP_FLG) -I $(INCS_DIR) -c $< -o $@

test: all
	bash ./tests/run_tests.sh

UNIT_TEST_DIR	= tests/unit
UNIT_TEST_BIN	= unit_tests
UNIT_TEST_SRC	= test_main.cpp test_parse.cpp test_replies.cpp test_client.cpp
UNIT_TEST_SRCS	= $(addprefix $(UNIT_TEST_DIR)/, $(UNIT_TEST_SRC))
UNIT_SRCS		= $(addprefix $(SRCS_DIR)/, $(SRC))

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
