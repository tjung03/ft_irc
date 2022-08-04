#include "server.hpp"

/*
	util 함수
*/
int	check_invalid_string(const std::string& pw)
{
	for (std::size_t i = 0; i < pw.size(); ++i)
	{
		if (!std::isalpha(pw[i]) && !std::isdigit(pw[i]))
			return (1);
	}
	return (0);
}

/*
	해당 cpp 파일에서만 사용하는 static 함수
*/
static void	exit_program(int print, const std::string& msg)
{
	if (print)
		std::cerr<<"ERROR\n"<<msg<<std::endl;
	exit(print);
}

static int	check_invalid_port(int port)
{
	if (1024 > port || 49151 < port)
		return (1);
	return (0);
}

/*
	server 클래스 생성자

	멤버변수 초기화(+연결형 소켓의 pollfd 구조체 값 초기화) 및 소켓 bind, listen
*/
server::server(char* av[])
	: _connect_pw(av[2]), _admin_pw("admin")
{
	this->_port = std::atoi(av[1]);
	if (check_invalid_port(this->_port))
		exit_program(1, UNAVAILABLE_PORT);
	if (check_invalid_string(this->_connect_pw))
		exit_program(1, UNAVAILABLE_PW);

	this->_connect_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (this->_connect_socket == -1)
		exit_program(1, FAILED_SOCK);

	// 소켓 옵션에서 bind 시에 local 주소를 재사용할 것인지 여부 true로 set
	int	optval = 1;
	if (setsockopt(this->_connect_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
		exit_program(1, FAILED_SET_SOCK);
	// 소켓 fd 를 비동기로 설정 (MacOS only)
	if (fcntl(this->_connect_socket, F_SETFL, O_NONBLOCK) == -1)
		exit_program(1, FAILED_NONBLOCK);

	struct sockaddr_in	serv_addr;
	std::memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(this->_port);

	if (bind(this->_connect_socket, reinterpret_cast<struct sockaddr *>(&serv_addr), sizeof(serv_addr)) == -1)
		exit_program(1, FAILED_BIND);
	if (listen(this->_connect_socket, SOMAXCONN) == -1)
		exit_program(1, FAILED_LISTEN);

	// 벡터 polls의 0번 인덱스 위치는 연결형 소켓을 위함 -> accept 여부 체크
	this->_polls.push_back(pollfd());
	this->_polls[0].fd = this->_connect_socket;
	this->_polls[0].events = POLLIN;
}

// 클래스 소멸자
server::~server(void) { }

/*
	server 클래스 멤버 함수
*/
std::string&	server::getConnectPW(void)
{
	return (this->_connect_pw);
}

std::string&	server::getAdminPW(void)
{
	return (this->_admin_pw);
}

int	server::getConnectSocket(void)
{
	return (this->_connect_socket);
}

std::vector<pollfd>&	server::getPolls(void)
{
	return (this->_polls);
}

std::map<int, user*>&	server::getUsers(void)
{
	return (this->_users);
}

std::map<std::string, std::vector<int> >&	server::getChannels(void)
{
	return (this->_channels);
}
