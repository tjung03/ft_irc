#ifndef SERVERHANDLER_HPP
# define SERVERHANDLER_HPP

# include "server.hpp"

# define RPL_WELCOME			"001"
# define ERR_UNKNOWNCOMMAND		"421"
# define ERR_ERRONEUSNICKNAME	"432"
# define ERR_NOTREGISTERED		"451"
# define ERR_NEEDMOREPARAMS		"461"
# define ERR_ALREADYREGISTRED	"462"
# define ERR_PASSWDMISMATCH		"464"

/*
	serverHandler : server 객체를 통제하는 목적
*/
class	serverHandler
{
private:
	server	_serv;	// 서버 객체

	serverHandler(void);
	serverHandler(const serverHandler& other);
	serverHandler&	operator=(const serverHandler& other);

public:
	serverHandler(char* av[]);
	~serverHandler(void);

	server&	getServ(void);

	void	serverOn(void);				// 서버 컨트롤 목적
	void	disconnect(int user_fd);	// 유저 연결 해제
	void	registrationUser(int user_fd, std::string& buffer);	// 유저 등록
	void	parsingMSG(int user_fd, std::string& buffer);		// 수신 메시지 파싱

	void	sendNumericReplies(int user_fd, std::string& numeric, std::string& user_name, std::string& replies);

	bool	commandPass(int user_fd, std::string& val);
	void	commandNick(int user_fd, std::string& val);
//	commandUser
//	commandPrivmsg
//	commandJoin
//	commandPart
//	commandKill
//	commandQuit
//	commandAdmin
//	commandList
};

#endif
