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

// 중복된 닉네임 있으면 1, 없으면 0 반환
static int	check_nick_used(std::map<int, user *>& users, int user_fd, const std::string& val)
{
	std::map<int, user *>::iterator	users_it = users.begin();
	std::map<int, user *>::iterator	users_end = users.end();

	for (; users_it != users_end; ++users_it)
	{
		if (!((users_it->second->getNick()).compare(val)))
		{
			if (users_it->second->getUserFd() == user_fd)
				return (0);
			return (1);
		}
	}
	return (0);
}

// 유저 정보를 한 줄로 문자열 반환
static std::string	get_full_user_info(const std::string& nick, const std::string& user, const std::string& host_name)
{
	std::string	ret = ":" + nick + "!" + user + "@" + host_name;
	return (ret);
}

// commandJoin 에서 다중 채널 접속할 때 구분자 처리
static std::string	remove_separator(const std::string& val)
{
	std::string	buffer;
	int			val_len = val.length();

	buffer.clear();
	for (int i = 0; i < val_len; ++i)
	{
		if (val[i] != ',' && val[i] != ' ')
			buffer += val[i];
	}
	return (buffer);
}


/*
	server 클래스 생성자, 소멸자
*/
serverHandler::serverHandler(char* av[])
	: _serv(av)
{
	std::cout<<"* launch server *"<<std::endl;
}

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
			break ;
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

	users::iterator	users_it;
	users::iterator	users_end;

	std::string	val;

	val.clear();
	for (; chanl_it != chanl_end; ++chanl_it)
	{
		users_it = this->_serv.getChannels()[chanl_it->first].begin();
		users_end = this->_serv.getChannels()[chanl_it->first].end();
		for (; users_it != users_end; ++users_it)
		{
			if (*users_it == user_fd)
			{
				val = "#" + chanl_it->first;
				commandPart(user_fd, val);
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
	else if (this->_serv.getUsers()[user_fd]->getIsPass() == true)
	{
		if (cmd == "NICK")
			commandNick(user_fd, buffer);
		else if (cmd == "USER")
		{
			if (this->_serv.getUsers()[user_fd]->getNick() == "")
				sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, "UNKNOWN", ":Not enough parameters :NICK <nickname>\r\n");
			else
				commandUser(user_fd, buffer);
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

	// USER NICK PRIVMSG JOIN PART KICK QUIT LIST
	std::map<int, user *>&	users = this->_serv.getUsers();

	if (cmd == "USER")
		sendNumericReplies(user_fd, ERR_ALREADYREGISTRED, users[user_fd]->getNick(), ":You may not reregister\r\n");
	else if (cmd == "NICK")
		commandNick(user_fd, buffer);
	else if (cmd == "PRIVMSG" || cmd == "NOTICE")
		commandPrivmsg(user_fd, buffer, cmd);
	else if (cmd == "JOIN")
		commandJoin(user_fd, buffer);
	else if (cmd == "PART")
		commandPart(user_fd, buffer);
	else if (cmd == "KICK")
		commandKick(user_fd, buffer);
	else if (cmd == "HOST")
		commandHost(user_fd, buffer);
	else if (cmd == "QUIT")
		commandQuit(user_fd);
	else if (cmd == "LIST")
		commandList(user_fd);
	else
		sendNumericReplies(user_fd, ERR_UNKNOWNCOMMAND, users[user_fd]->getNick(), ":Unknown command :" + cmd + " " + buffer + "\r\n");
}

void	serverHandler::sendNumericReplies(int user_fd, const std::string& numeric, const std::string& user_name, const std::string& replies)
{
	std::string	numeric_replies = numeric + " " + user_name + " " + replies;
	send(user_fd, numeric_replies.c_str(), numeric_replies.length(), 0);
}

void	serverHandler::check_user_info(user* user_, int user_fd)
{
	if (user_->getNick() == "")
		sendNumericReplies(user_fd, ERR_NOTREGISTERED, "UNKNOWN", ":You have not registered: Check nickname\r\n");
	else
	{
		if (user_->getUser() == "")
			sendNumericReplies(user_fd, ERR_NOTREGISTERED, user_->getNick(), ":You have not registered: Check username\r\n");
		else
		{
			user_->setTrueSignIn();	// Succeed SignIn
			sendNumericReplies(user_fd, RPL_WELCOME, user_->getNick(), ":Welcome to the Internet Relay Network\r\n");
			std::string	temp = get_full_user_info(user_->getNick(), user_->getUser(), user_->getHostName()) + "\r\n";
			send(user_fd, temp.c_str(), temp.length(), 0);
		}
	}
}

void	serverHandler::broadcast_message(int user_fd, const std::string& ch_name, const std::string& cmd)
{
	std::map<int, user *>&	users_ = this->_serv.getUsers();

	std::string	msg = get_full_user_info(users_[user_fd]->getNick(), users_[user_fd]->getUser(), users_[user_fd]->getHostName());
	msg = msg + " " + cmd + " #" + ch_name + "\r\n";

	std::vector<int>::iterator	users_fd_it = this->_serv.getChannels()[ch_name].begin();
	std::vector<int>::iterator	users_fd_end = this->_serv.getChannels()[ch_name].end();
	for (; users_fd_it != users_fd_end; ++users_fd_it)
	{
		if (*users_fd_it != user_fd)
			send(*users_fd_it, msg.c_str(), msg.length(), 0);
	}
}


/*
	command 함수 : PASS NICK USER PRIVMSG JOIN PART KICK QUIT (HOST) LIST
*/
// 서버 연결 비밀번호 입력 (체크)
bool	serverHandler::commandPass(int user_fd, const std::string& val)
{
	if (check_invalid_string(val))
		sendNumericReplies(user_fd, ERR_PASSWDMISMATCH, "UNKNOWN", ":Password incorrect :Pleas, alphabet or numbers\r\n");
	else
	{
		if (!val.compare(this->_serv.getAdminPW()))
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

// 닉 정보 입력, 기존 유저면 닉 변경
void	serverHandler::commandNick(int user_fd, const std::string& val)
{
	if (check_invalid_string(val))
		sendNumericReplies(user_fd, ERR_ERRONEUSNICKNAME, "UNKNOWN", ":Erroneus nickname :Pleas, alphabet or numbers\r\n");
	else
	{
		if (!check_nick_used(this->_serv.getUsers(), user_fd, val))
			this->_serv.getUsers()[user_fd]->setNick(val);
		else
		{
			if (this->_serv.getUsers()[user_fd]->getNick() == "")
				sendNumericReplies(user_fd, ERR_NICKNAMEINUSE, "UNKNOWN", ":Nickname is already in use :" + val + "\r\n");
			else
				sendNumericReplies(user_fd, ERR_NICKNAMEINUSE, this->_serv.getUsers()[user_fd]->getNick(), ":Nickname is already in use :" + val + "\r\n");
		}
	}
}

// 유저 정보 입력
void	serverHandler::commandUser(int user_fd, const std::string& val)
{
	if (val.find(' ', 0) != std::string::npos)
		sendNumericReplies(user_fd, ERR_USERSDISABLED, this->_serv.getUsers()[user_fd]->getNick(), ":USERS has been disabled :No space\r\n");
	else if (check_invalid_string(val))
		sendNumericReplies(user_fd, ERR_USERSDISABLED, this->_serv.getUsers()[user_fd]->getNick(), ":USERS has been disabled :Please, re-enter\r\n");
	else
	{
		this->_serv.getUsers()[user_fd]->setUser(val);
		check_user_info(this->_serv.getUsers()[user_fd], user_fd);
	}
}

// 채널 or 유저(닉) 메세지 보내기
void	serverHandler::commandPrivmsg(int user_fd, std::string& val, const std::string& cmd)
{
	typedef std::map<int, user *>						t_users;
	typedef std::map<std::string, std::vector<int> >	t_chanls;

	t_users&	users = this->_serv.getUsers();
	std::size_t	separator;
	std::string	receiver;

	separator = val.find(' ', 0);
	if (!val.size() || separator == std::string::npos)
	{
		sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, users[user_fd]->getNick(), ":Not enough parameters :PRIVMSG <nickname|#channel> <message>\r\n");
		return ;
	}
	receiver = val.substr(0, separator);
	val.erase(0, separator + 1);
	val = val + "\r\n";
	// 채널에 메세지를 보낼 때
	if (receiver[0] == '#')
	{
		t_chanls&	chanls = this->_serv.getChannels();

		receiver.erase(0, 1);
		if (chanls.find(receiver) == chanls.end())
			sendNumericReplies(user_fd, ERR_NOSUCHCHANNEL, users[user_fd]->getNick(), ":No such #" + receiver + " :Check 'LIST'\r\n");
		else
		{
			std::vector<int>::iterator	chanls_it = chanls[receiver].begin();
			std::vector<int>::iterator	chanls_end = chanls[receiver].end();
			for (; chanls_it != chanls_end; ++chanls_it)
			{
				if (*chanls_it == user_fd)
				{
					val = get_full_user_info(users[user_fd]->getNick(), users[user_fd]->getUser(), users[user_fd]->getHostName()) \
						+ " " + cmd + " #" + receiver + " " + val;
					chanls_it = chanls[receiver].begin();
					chanls_end = chanls[receiver].end();
					for (; chanls_it != chanls_end; ++chanls_it)
					{
						if (*chanls_it != user_fd)
							send(*chanls_it, val.c_str(), val.length(), 0);
					}
					return ;
				}
			}
			sendNumericReplies(user_fd, ERR_NOTONCHANNEL, users[user_fd]->getNick(), ":You're not on that #" + receiver + "\r\n");
		}
		return ;
	}
	// 유저에게 메세지를 보낼 때
	t_users::iterator	users_it = users.begin();
	t_users::iterator	users_end = users.end();
	for (; users_it != users_end; ++users_it)
	{
		if (users_it->second->getNick() == receiver)
		{
			val = get_full_user_info(users[user_fd]->getNick(), users[user_fd]->getUser(), users[user_fd]->getHostName()) \
				+ " PRIVMSG " + users[users_it->first]->getNick() + " :" + val;
			send(users_it->second->getUserFd(), val.c_str(), val.length(), 0);
			return ;
		}
	}
	sendNumericReplies(user_fd, ERR_NOSUCHNICK, users[user_fd]->getNick(), ":No such nickname :" + receiver + "\r\n");
}

// 채널 참가하기
void	serverHandler::commandJoin(int user_fd, std::string& val)
{
	typedef std::map<int, user *>						t_users;
	typedef std::map<std::string, std::vector<int> >	t_chanls;

	t_users&	users = this->_serv.getUsers();
	t_chanls&	channels = this->_serv.getChannels();

	std::string	chanl;
	std::size_t	sharps = 0;
	int			unit;

	val = remove_separator(val);
	while (1)
	{
		if (sharps == std::string::npos)
			break ;
		unit = sharps;
		sharps = val.find('#', sharps + 1);
		chanl.clear();
		chanl = val.substr(unit, sharps - unit);
		if (chanl.at(0) != '#')
		{
			sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, users[user_fd]->getNick(), ":Not enough parameters :Channel name must begin '#'\r\n");
			break ;
		}
		chanl.erase(0, 1);
		if (!chanl.size())
		{
			sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, users[user_fd]->getNick(), ":Not enough parameters :Please, enter channel name\r\n");
			break ;
		}
		t_chanls::iterator	chanls_it = channels.begin();
		t_chanls::iterator	chanls_end = channels.end();
		for (; chanls_it != chanls_end; ++chanls_it)
		{
			if (chanls_it->first == chanl)
			{
				std::vector<int>::iterator	users_it = channels[chanls_it->first].begin();
				std::vector<int>::iterator	users_end = channels[chanls_it->first].end();
				for (; users_it != users_end; ++users_it)
				{
					if (*users_it == user_fd)
					{
						sendNumericReplies(user_fd, ERR_USERONCHANNEL, users[user_fd]->getNick(), ":is already on #" + chanls_it->first + "\r\n");
						break ;
					}
				}
				// join channel
				if (users_it == users_end)
				{
					chanls_it->second.push_back(user_fd);
					users[user_fd]->setChanlHost(chanls_it->first, false);
					sendNumericReplies(user_fd, RPL_WELCOME, users[user_fd]->getNick(), ":Succeeded JOIN to #" + chanls_it->first + "\r\n");
					broadcast_message(user_fd, chanls_it->first, "JOIN");
				}
				break ;
			}
		}
		// create new channel
		if (chanls_it == chanls_end)
		{
			channels[chanl].push_back(user_fd);
			users[user_fd]->setChanlHost(chanl, true);
			sendNumericReplies(user_fd, RPL_WELCOME, users[user_fd]->getNick(), ":Succeeded JOIN to #" + chanl + " :Create new channel"+ "\r\n");
		}
	}
}

// 채널 나가기
void	serverHandler::commandPart(int user_fd, std::string& val)
{
	typedef std::map<int, user *>						t_users;
	typedef std::map<std::string, std::vector<int> >	t_chanls;

	t_users&	users = this->_serv.getUsers();
	t_chanls&	channels = this->_serv.getChannels();

	std::string	chanl;
	std::size_t	sharps = 0;
	int			unit;

	val = remove_separator(val);
	while (1)
	{
		if (sharps == std::string::npos)
			break ;
		unit = sharps;
		sharps = val.find('#', sharps + 1);
		chanl.clear();
		chanl = val.substr(unit, sharps - unit);
		if (chanl.at(0) != '#')
		{
			sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, users[user_fd]->getNick(), ":Not enough parameters :Channel name must begin '#'\r\n");
			break ;
		}
		chanl.erase(0, 1);
		if (!chanl.size())
		{
			sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, users[user_fd]->getNick(), ":Not enough parameters :Please, enter channel name\r\n");
			break ;
		}
		if (channels.find(chanl) != channels.end())
		{
			std::vector<int>::iterator	users_it = channels[chanl].begin();
			std::vector<int>::iterator	users_end = channels[chanl].end();
			for (; users_it != users_end; ++users_it)
			{
				if (*users_it == user_fd)
				{
					channels[chanl].erase(users_it);
					users[user_fd]->getChanlHost().erase(chanl);
					sendNumericReplies(user_fd, RPL_WELCOME, users[user_fd]->getNick(), ":Leave channel #" + chanl + "\r\n");
					broadcast_message(user_fd, chanl, "PART");
					break ;
				}
			}
			if (users_it == users_end)
				sendNumericReplies(user_fd, ERR_NOTONCHANNEL, users[user_fd]->getNick(), ":You're not on that #" + chanl + "\r\n");
		}
		else
			sendNumericReplies(user_fd, ERR_NOSUCHCHANNEL, users[user_fd]->getNick(), ":No such #" + chanl + " :Check 'LIST'\r\n");
	}
}

// 추방 명령어 admin host 용
void	serverHandler::commandKick(int user_fd, std::string& val)
{
	typedef std::map<std::string, std::vector<int> >	t_chanls;
	typedef std::map<int, user *>						t_users;

	t_chanls&	channels = this->_serv.getChannels();
	t_users&	users = this->_serv.getUsers();

	std::size_t	separator;
	std::string	chanl;

	separator = val.find('#', 0);
	if (!val.size() || separator == std::string::npos)
	{
		sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, users[user_fd]->getNick(), ":Not enough parameters :KICK <#channel> <nickname>\r\n");
		return ;
	}
	separator = val.find(' ', separator + 1);
	if (separator == std::string::npos)
	{
		sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, users[user_fd]->getNick(), ":Not enough parameters :KICK <#channel> <nickname>\r\n");
		return ;
	}
	chanl = val.substr(0, separator);
	val.erase(0, separator + 1);
	if (chanl[0] == '#')
	{
		std::string	msg;

		chanl.erase(0, 1);
		if (channels.find(chanl) == channels.end())
			sendNumericReplies(user_fd, ERR_NOSUCHCHANNEL, users[user_fd]->getNick(), ":No such #" + chanl + " :Check target infomation\r\n");
		else
		{
			std::map<std::string, bool>::iterator	check = users[user_fd]->getChanlHost().find(chanl);
			if ((users[user_fd]->getIsAdmin() != true) \
				&& ((check == users[user_fd]->getChanlHost().end()) || (check != users[user_fd]->getChanlHost().end() && (check->second == false))))
			{
				sendNumericReplies(user_fd, ERR_NOPRIVILEGES, users[user_fd]->getNick(), ":Permission Denied :You have no privileges\r\n");
				return ;
			}

			std::vector<int>::iterator	chanls_it = channels[chanl].begin();
			std::vector<int>::iterator	chanls_end = channels[chanl].end();
			for (; chanls_it != chanls_end; ++chanls_it)
			{
				if (users[*chanls_it]->getNick() == val)
				{
					if (users[*chanls_it]->getIsAdmin() == true)
						sendNumericReplies(user_fd, ERR_NOPRIVILEGES, users[user_fd]->getNick(), ":Permission Denied :Target is a admin\r\n");
					else
					{
						if ((users[user_fd]->getIsAdmin() == false) && (users[*chanls_it]->getChanlHost().find(chanl)->second == true))
							sendNumericReplies(user_fd, ERR_NOPRIVILEGES, users[user_fd]->getNick(), ":Permission Denied :Target is a host\r\n");
						else
						{
							sendNumericReplies(user_fd, RPL_WELCOME, users[user_fd]->getNick(), ":Succeed KICK :'" + val + "' at #" + chanl + "\r\n");
							msg = "You were expelled from #" + chanl + "\r\n";
							send(*chanls_it, msg.c_str(), msg.length(), 0);

							msg = "#" + chanl;
							commandPart(*chanls_it, msg); msg.clear();
						}
					}
					return ;
				}
			}
			sendNumericReplies(user_fd, ERR_NOTONCHANNEL, users[user_fd]->getNick(), ":'" + val + "' not on that #" + chanl + "\r\n");
		}
		return ;
	}
	sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, "ADMIN", ":Not enough parameters :KICK <#channel> <nickname>\r\n");
}

// 호스트 임명/해제 admin 용
void	serverHandler::commandHost(int user_fd, std::string& val)
{
	typedef std::map<std::string, std::vector<int> >	t_chanls;
	typedef std::map<int, user *>						t_users;

	t_chanls&	channels = this->_serv.getChannels();
	t_users&	users = this->_serv.getUsers();

	if (users[user_fd]->getIsAdmin() == false)
	{
		sendNumericReplies(user_fd, ERR_NOPRIVILEGES, users[user_fd]->getNick(), ":Permission Denied\r\n");
		return ;
	}

	std::size_t	separator;
	std::string	chanl;

	separator = val.find('#', 0);
	if (!val.size() || separator == std::string::npos)
	{
		sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, "ADMIN", ":Not enough parameters :HOST <#channel> <nickname>\r\n");
		return ;
	}
	separator = val.find(' ', separator + 1);
	if (separator == std::string::npos)
	{
		sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, "ADMIN", ":Not enough parameters :HOST <#channel> <nickname>\r\n");
		return ;
	}
	chanl = val.substr(0, separator);
	val.erase(0, separator + 1);
	if (chanl[0] == '#')
	{
		std::string msg;

		chanl.erase(0, 1);
		if (channels.find(chanl) == channels.end())
			sendNumericReplies(user_fd, ERR_NOSUCHCHANNEL, "ADMIN", ":No such #" + chanl + " :Check target infomation\r\n");
		else
		{
			std::vector<int>::iterator	chanls_it = channels[chanl].begin();
			std::vector<int>::iterator	chanls_end = channels[chanl].end();
			for (; chanls_it != chanls_end; ++chanls_it)
			{
				if (users[*chanls_it]->getNick() == val)
				{
					if (users[*chanls_it]->getChanlHost()[chanl] == true)
					{
						users[*chanls_it]->setChanlHost(chanl, false);
						msg = "Admin changed :You are not a host\r\n";
					}
					else
					{
						users[*chanls_it]->setChanlHost(chanl, true);
						msg = "Admin changed :You are a host\r\n";
					}
					sendNumericReplies(user_fd, RPL_WELCOME, "ADMIN", ":Succeed HOST :'" + val + "' at #" + chanl + "\r\n");
					send(*chanls_it, msg.c_str(), msg.length(), 0);
					return ;
				}
			}
			sendNumericReplies(user_fd, ERR_NOTONCHANNEL, "ADMIN", ":'" + val + "' not on that #" + chanl + "\r\n");
		}
		return ;
	}
	sendNumericReplies(user_fd, ERR_NEEDMOREPARAMS, "ADMIN", ":Not enough parameters :HOST <#channel> <nickname>\r\n");
}

// 유저가 참여하고 있는 채널 리스트업 + 전체 채널 리스트업
void	serverHandler::commandList(int user_fd)
{
	sendNumericReplies(user_fd, RPL_WELCOME, this->_serv.getUsers()[user_fd]->getNick(), ":Succeeded list-up\r\n");

	std::string	list = "<- Full Channel List ->\r\n";
	send(user_fd, list.c_str(), list.length(), 0);

	list.clear();
	std::map<std::string, std::vector<int> >::iterator	chanls_it = this->_serv.getChannels().begin();
	std::map<std::string, std::vector<int> >::iterator	chanls_end = this->_serv.getChannels().end();
	for (; chanls_it != chanls_end; ++chanls_it)
	{
		list += "#" + chanls_it->first + "\r\n";
		send(user_fd, list.c_str(), list.length(), 0);
		list.clear();
	}

	list = "<- User Channel List ->\r\n";
	send(user_fd, list.c_str(), list.length(), 0);

	list.clear();
	std::map<std::string, bool>::iterator	user_chanls_it = (this->_serv.getUsers()[user_fd])->getChanlHost().begin();
	std::map<std::string, bool>::iterator	user_chanls_end = (this->_serv.getUsers()[user_fd])->getChanlHost().end();
	for (; user_chanls_it != user_chanls_end; ++user_chanls_it)
	{
		list += "#" + user_chanls_it->first + "\r\n";
		send(user_fd, list.c_str(), list.length(), 0);
		list.clear();
	}
}

// 서버 연결 해제
void	serverHandler::commandQuit(int user_fd)
{
	sendNumericReplies(user_fd, RPL_WELCOME, this->_serv.getUsers()[user_fd]->getNick(), ":Goodbye\r\n");
	disconnect(user_fd);
}
