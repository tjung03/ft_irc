NAME =		ircserv

CPP = 		c++
CPPFLAGS =	-fsanitize=address -pedantic -std=c++98 -W -Wall -Wextra -Werror

SRCS =		./main.cpp \
			./srcs/serverHandler.cpp \
			./srcs/server.cpp \
			./srcs/user.cpp

OBJS =		$(SRCS:.cpp=.o)

.PHONY:		all clean fclean re

%.o : %.cpp
			$(CPP) $(CPPFLAGS) -c $< -o $@

all:		$(NAME)

$(NAME):	$(OBJS)
			@echo "\n\033[0;33mCompiling..."
			$(CPP) $(CPPFLAGS) -o $(NAME) $(OBJS)
			@echo "\033[0m"

clean:
			@echo "\n\033[0;31mCleaning..."
			rm -f $(OBJS)
			@echo "\033[0m"

fclean:		clean
			@echo "\033[0;31mRemoving executable..."
			rm -f $(NAME)
			@echo "\033[0m"

re: 		fclean all
