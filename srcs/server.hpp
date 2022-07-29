#ifndef SERVER_CPP
# define SERVER_CPP

# include <iostream>
# include <vector>
# include <map>
# include <string>
# include <poll.h>
# include "error_msg.hpp"

# include <sstream>
# include <cstdlib>
# include <cctype>
# include <fcntl.h>

# include <sys/socket.h>

/*
	server : irc server 클래스
*/
class	server
{
private:
	std::string	_connect_pw;
	std::string	_admin_pw;

	int	_port;
	int	_connect_socket;

	std::vector<pollfd>							_polls;
	std::map<int, user*>						_users;
	std::map<std::string, std::vector<int> >	_channels;

	server(void);
	server(const server& other);
	server&	operater=(const server& other);

public:
	server(char* av[]);

	std::vector<pollfd>&	getPolls(void) const;
};

#endif
