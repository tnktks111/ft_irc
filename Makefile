NAME = ircserv
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic-errors -MMD -MP

INCLUDES = -I $(INC_DIR)

BASENAMES = main ACommand Channel Client CommandContext CommandDispatcher InviteCommand JoinCommand KickCommand Message ModeCommand NickCommand PartCommand PassCommand PingCommand PrivMsgCommand QuitCommand ReplyBuilder ResponseSink Server ServerContext TopicCommand UserCommand

SRCS = $(addprefix $(SRC_DIR)/, $(addsuffix .cpp, $(BASENAMES)))
OBJS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(BASENAMES)))
DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
