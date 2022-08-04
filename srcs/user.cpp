#include "user.hpp"
#include "arpa/inet.h"

/*
	user 클래스 생성자
*/
user::user(int user_fd, struct sockaddr_in& client_addr)
	: _user_fd(user_fd), _client_addr(client_addr), _nick(""), _user(""), _is_pass(false), _is_admin(false), _sign_in(false)
{
	fcntl(this->_user_fd, F_SETFL, O_NONBLOCK);
	// 유저 정보(설정) 초기화 + 호스트네임 얻기
	this->_host_name = inet_ntoa(this->_client_addr.sin_addr);
}

// 소멸자
user::~user(void) { }

/*
	user 클래스 멤버 함수
*/
int	user::getUserFd(void)
{
	return (this->_user_fd);
}

struct sockaddr_in&	user::getClientAddr(void)
{
	return (this->_client_addr);
}

std::string&	user::getNick(void)
{
	return (this->_nick);
}

std::string&	user::getUser(void)
{
	return (this->_user);
}

std::string&	user::getHostName(void)
{
	return (this->_host_name);
}

bool	user::getIsPass(void)
{
	return (this->_is_pass);
}

bool	user::getIsAdmin(void)
{
	return (this->_is_admin);
}

bool	user::getSignIn(void)
{
	return (this->_sign_in);
}

std::map<std::string, bool>&	user::getChanlHost(void)
{
	return (this->_chanl_host);
}

void	user::setNick(const std::string& nick)
{
	this->_nick = nick;
}

void	user::setUser(const std::string& user)
{
	this->_user = user;
}

void	user::setTruePass(void)
{
	this->_is_pass = true;
}

void	user::setTrueAdmin(void)
{
	this->_is_admin = true;
}

void	user::setTrueSignIn(void)
{
	this->_sign_in = true;
}

void	user::setChanlHost(const std::string& chanl, bool host)
{
	this->_chanl_host[chanl] = host;
}
