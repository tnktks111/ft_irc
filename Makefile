NAME = ircserv
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -pedantic-errors -MMD -MP

INCLUDES = -I $(INC_DIR) -I $(INC_DIR)/core -I $(INC_DIR)/commands -I $(INC_DIR)/domain -I $(INC_DIR)/response

SRCS = \
	$(SRC_DIR)/app/main.cpp \
	$(SRC_DIR)/core/CommandContext.cpp \
	$(SRC_DIR)/core/CommandDispatcher.cpp \
	$(SRC_DIR)/core/Server.cpp \
	$(SRC_DIR)/core/ServerContext.cpp \
	$(SRC_DIR)/commands/ACommand.cpp \
	$(SRC_DIR)/commands/InviteCommand.cpp \
	$(SRC_DIR)/commands/JoinCommand.cpp \
	$(SRC_DIR)/commands/KickCommand.cpp \
	$(SRC_DIR)/commands/ModeCommand.cpp \
	$(SRC_DIR)/commands/NickCommand.cpp \
	$(SRC_DIR)/commands/PartCommand.cpp \
	$(SRC_DIR)/commands/PassCommand.cpp \
	$(SRC_DIR)/commands/PingCommand.cpp \
	$(SRC_DIR)/commands/PrivMsgCommand.cpp \
	$(SRC_DIR)/commands/QuitCommand.cpp \
	$(SRC_DIR)/commands/TopicCommand.cpp \
	$(SRC_DIR)/commands/UserCommand.cpp \
	$(SRC_DIR)/domain/Channel.cpp \
	$(SRC_DIR)/domain/Client.cpp \
	$(SRC_DIR)/domain/Message.cpp \
	$(SRC_DIR)/response/ReplyBuilder.cpp \
	$(SRC_DIR)/response/ResponseSink.cpp

OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

fmt:
	@echo "Formatting code..."
	@find . -type f \( -name "*.cpp" -o -name "*.hpp" \) -not -path "./.*" -exec clang-format -i {} +
	@echo "Done."

-include $(DEPS)

.PHONY: all clean fclean re fmt
