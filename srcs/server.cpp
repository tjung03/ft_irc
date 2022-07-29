#include "server.hpp"

static void	exit_program(int print, std::string& msg)
{
	if (print)
		std::cout<<"ERROR\n"<<msg<<std::endl;
	exit(print);
}

static int	check_invalid_port(int port)
{
	if (1024 > port || 49151 < port)
		return (1);
	return (0);
}

static int	check_invalid_password(std::string& pw)
{
	for (int i = 0; i < pw.size(); ++i)
	{
		if (!std::isalpha(pw[i]) && !std::isdigit(pw[i]))
			return (1);
	}
	return (0);
}

server::server(char* av[])
	: _connect_pw(av[2]), _admin_pw("admin")
{
	this->_port = std::atoi(av[1]);
	if (check_invalid_port(this->_port))
		exit_program(1, UNAVAILABLE_PORT);
	if (check_invalid_password(this->_connect_pw))
		exit_program(1, UNAVAILABLE_PW);

	this->_connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->_connect_socket == -1)
		exit_program(1, FAILED_SOCK);
	int	optval = 1;
	if (setsockopt(this->_connect_socket, SOL_SOCKET, SOL_REUSEADDR, &optval, sizeof(optval)) == -1)
		exit_program(1, FAILED_SET_SOCK);
	if (fcntl(this->_connect_socket, F_SETFL, O_NONBLOCK) == -1)
		exit_program(1, FAILED_NONBLOCK);
}
