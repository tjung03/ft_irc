#ifndef SERVER_CPP
# define SERVER_CPP

# include <iostream>
# include <vector>
# include <map>
# include <string>
# include <poll.h>
# include <sys/socket.h>
# include <arpa/inet.h>

# include <unistd.h>
# include <sstream>
# include <cstdlib>
# include <cctype>
# include <cstring>
# include <fcntl.h>

# include "error_msg.hpp"
# include "user.hpp"

int	check_invalid_password(std::string& pw);

class user;

/*
	server : irc server 클래스
*/
class	server
{
private:
	std::string	_connect_pw;	// 서버 비밀번호
	std::string	_admin_pw;		// 전체 관리자 비밀번호

	int	_port;				// connect 서버 포트
	int	_connect_socket;	// connect 소켓 fd

	std::vector<pollfd>							_polls;		// 단일 프로세스 다중 파일 입출력 체크하기 위한 자료구조(순회하면서 socket events 발생을 감시)
	std::map<int, user *>						_users;		// 등록 유저 자료구조
	std::map<std::string, std::vector<int> >	_channels;	// 채팅 채널 자료구조

	server(void);
	server(const server& other);
	server&	operator=(const server& other);

public:
	server(char* av[]);
	~server(void);

	// getter 함수
	int											getConnectSocket(void);
	std::vector<pollfd>&						getPolls(void);
	std::map<int, user *>&						getUsers(void);
	std::map<std::string, std::vector<int> >&	getChannels(void);
};

#endif
