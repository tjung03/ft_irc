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
				commandPart(user_fd, '#' + chanl_it->first);//
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
	// PASS NICK USER
	if (cmd == "PASS")
	{
		if (commandPass(user_fd, buffer) == true)
			this->_serv.getUsers()[user_fd]->setTruePass();
	}
	if (this->_serv.getUsers()[user_fd]->getIsPass() == true)
	{
		if (cmd == "NICK")
			commandNick(user_fd, buffer);
		else if (cmd == "USER")
		{
			if (this->_serv.getUsers()[user_fd]->getNick() == "")
				sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, "UNKNOWN", ":Not enough parameters :NICK <nickname>\r\n");
			commandUser(user_fd, buffer);//
		}
		else
		{
			if (this->_serv.getUsers()[user_fd]->getNick() == "")
				sendNumericReplies(user_fd, ERR_UNKNOWNCOMMAND, "UNKNOWN", ":Unknown command :" + cmd + " " + buffer + "\r\n");
			else
				sendNumericReplies(user_fd, ERR_UNKNOWNCOMMAND, this->_serv.getUsers()[user_fd]->getNick(), ":Unknown command :" + cmd + " " + buffer + "\r\n");
		}
	}
	else
		sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, "UNKNOWN", ":Not enough parameters :PASS <password>\r\n");
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

	// USER NICK PRIVMSG JOIN PART KILL QUIT ADMIN LIST
	std::map<int, user *>&	users = this->_serv.getUsers();

	if (cmd == "USER")
		sendNumericReplies(user_fd, ERR_ALREADYREGISTRED, users[user_fd]->getNick(), ":You may not reregister\r\n");
	else if (cmd == "NICK")
		commandNick(user_fd, buffer);
	else if (cmd == "PRIVMSG")
		commandPrivmsg(user_fd, buffer);//
	else if (cmd == "JOIN")
		commandJoin(user_fd, buffer);//
	else if (cmd == "PART")
		commandPart(user_fd, buffer);//
	else if (cmd == "KILL")
		commandKill(user_fd, buffer);//
	else if (cmd == "QUIT")
		commandQuit(user_fd);//
	else if (cmd == "ADMIN")
		commandAdmin(user_fd, buffer);//
	else if (cmd == "LIST")
		commandList(user_fd);//
	else
		sendNumericReplies(user_fd, ERR_UNKNOWNCOMMAND, users[user_fd]->getNick(), ":Unknown command :" + cmd + " " + buffer + "\r\n")
}

void	serverHandler::sendNumericReplies(int user_fd, std::string& numeric, std::string& user_name, std::string& replies)
{
	std::string	numeric_replies = numeric + " " + user_name + " " + replies;
	send(user_fd, numeric_replies.c_str(), numeric_replies.length(), 0);
}

/*
	command 함수 : PASS NICK USER PRIVMSG JOIN PART KILL QUIT ADMIN LIST
*/
bool	serverHandler::commandPass(int user_fd, std::string& val)
{
	if (check_invalid_password(val))
		sendNumericReplies(user_fd, ERR_PASSWDMISMATCH, "UNKNOWN", ":Password incorrect :Pleas, alphabet or numbers\r\n");
	else
	{
		if (!val.compare(this->_serv.getAdminPW())
			this->_serv.getUsers()[user_fd]->setTrueAdmin();
		else if (val.compare(this->_serv.getConnectPW()) != 0)
		{
			sendNumericReplies(user_fd, ERR_PASSWDMISMATCH, "UNKNOWN", ":Password incorrect :Pleas, re-enter\r\n");
			return (false);
		}
		return (true);
	}
	return (false);
}

void	serverHandler::commandNick(int user_fd, std::string& val)
{
	if (check_invalid_password(val))
		sendNumericReplies(user_fd, ERR_ERRONEUSNICKNAME, "UNKNOWN", ":Erroneus nickname :Pleas, alphabet or numbers\r\n");
	// else
	// 여기서부터 시작-> 알파벳, 숫자로 들어왔을 때 닉넴 처리 등록
}
