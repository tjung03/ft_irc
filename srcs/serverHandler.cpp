#include "serverHandler.hpp"

/*
	해당 cpp 파일에서만 사용하는 static 함수
*/
// 버퍼에 클라이언트 소켓 송신 메시지 수신
static std::string receive_msg(int fd)
{
	char	buffer[4096] = { 0, };

	if (recv(fd, &buffer, 4095, 0) == -1)
		return ("");
	return (std::string(buffer));
}

/*
	server 클래스 생성자
*/
serverHandler::serverHandler(char* av[])
	: _serv(av)
{
	std::cout<<"* launch server *"<<std::endl;
}

// 클래스 소멸자
serverHandler::~serverHandler(void)
{
	std::cout<<"* finish server *"<<std::endl;
}

/*
	server 클래스 멤버 함수
*/
server&	serverHandler::getServ(void)
{
	return (this->_serv);
}

// server/client 연결
void	serverHandler::serverOn(void)
{
	std::vector<pollfd>&	polls = this->_serv.getPolls();

	if (poll(&polls[0], polls.size(), -1) == -1)
		return ;
	if ((polls[0].revents & POLLIN) == POLLIN)
	{
		struct sockaddr_in	client_addr;
		socklen_t			client_addr_size = sizeof(client_addr);
		int					user_fd;

		user_fd = accept(this->_serv.getConnectSocket(), reinterpret_cast<struct sockaddr *>(&client_addr), &client_addr_size);
		if (user_fd == -1)
			return ;
		this->_serv.getUsers()[user_fd] = new user(user_fd, client_addr);

		polls.push_back(pollfd());
		polls.back().fd = user_fd;
		polls.back().events = POLLIN;
		return ;
	}

	std::string	msg;

	for (std::vector<pollfd>::iterator it = polls.begin() + 1; it != polls.end(); ++it)
	{
		if (it->revents & (POLLHUP | POLLNVAL | POLLERR))
		{
			disconnect(it->fd);
			return ;
		}
		if ((it->revents & POLLIN) == POLLIN)
		{
			msg = receive_msg(it->fd);
			if (msg.size() == 0)
				return ;

			std::stringstream	msg_stream(msg);
			std::string			buffer;

			while (std::getline(msg_stream, buffer))
			{
				if (buffer[buffer.size() - 1] == '\r')
					buffer.erase(buffer.size() - 1, 1);
				if (!(((this->_serv).getUsers())[it->fd])->getSignIn())
					this->registrationUser(it->fd, buffer);
				else
					this->parsingMSG(it->fd, buffer);
				buffer.clear();
			}
		}
	}
}

void	serverHandler::disconnect(int user_fd)
{
	typedef std::map<std::string, std::vector<int> >	channels;
	typedef std::vector<int>							users;
	typedef std::vector<pollfd>							polls;

	channels::iterator	chanl_it = this->_serv.getChannels().begin();
	channels::iterator	chanl_end = this->_serv.getChannels().end();
	users::iterator		users_it;
	users::iterator		users_end;

	for (; chanl_it != chanl_end; ++chanl_it)
	{
		users_it = this->_serv.getChannels()[chanl_it->first].begin();
		users_end = this->_serv.getChannels()[chanl_it->first].end();
		for (; users_it != users_end; ++users_it)
		{
			if (*users_it == user_fd)
			{
				// commandPart(user_fd, '#' + chanl_it->first);
				break ;
			}
		}
	}

	polls::iterator	polls_it = this->_serv.getPolls().begin() + 1;
	polls::iterator	polls_end = this->_serv.getPolls().end();
	for (; polls_it != polls_end; ++polls_it)
	{
		if (polls_it->fd == user_fd)
		{
			this->_serv.getPolls().erase(polls_it);
			delete (this->_serv.getUsers()[user_fd]);
			this->_serv.getUsers().erase(user_fd);
			close(user_fd);
			break ;
		}
	}
}

void	serverHandler::registrationUser(int user_fd, std::string& buffer)
{
	std::string	cmd;
	std::size_t	separator = 0;

	separator = buffer.find(' ', 0);
	cmd = buffer.substr(0, separator);
	if (separator == std::string::npos)
		buffer = "";
	else
		buffer.erase(0, separator + 1);

	// 서버 연결 비밀번호 입력 후 닉네임, 아이디 입력 받자.
	if (cmd == "PASS")
	{
		// commandPass(user_fd, buffer);
		if (cmd == "NICK")
			; // commandNick(user_fd, buffer);
		else if (cmd == "USER")
			; // commandUser(user_fd, buffer);
		else
			; // sendNumericReplies(user_fd, ERR_NOTREGISTERED, "UNKNOWN", "Check NICK USER\r\n")
	}
	else
		; // sendNumericReplies(user_fd, ERR_PASSWDMISMATCH, "UNKNOWN", "Check PASS\r\n")
}

void	serverHandler::parsingMSG(int user_fd, std::string& buffer)
{
	std::string	cmd;
	std::size_t	separator = 0;

	separator = buffer.find(' ', 0);
	cmd = buffer.substr(0, separator);
	if (separator == std::string::npos)
		buffer = "";
	else
		buffer.erase(0, separator + 1);

	// PRIVMSG LIST JOIN QUIT PART ADMIN KILL NICK USER
//	if (cmd == "PRIVMSG")
//		;
//	else if (cmd == "...")
//		;
//	...
//		;
//	else
//		sendNumericReplies(알 수 없는 명령어 처리)
}
