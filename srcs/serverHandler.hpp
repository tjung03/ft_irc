#ifndef SERVERHANDLER_HPP
# define SERVERHANDLER_HPP

# include "server.hpp"

# define RPL_WELCOME			"001"
# define ERR_UNKNOWNCOMMAND		"421"
# define ERR_ERRONEUSNICKNAME	"432"
# define ERR_NICKNAMEINUSE		"433"
# define ERR_USERSDISABLED		"446"
# define ERR_NOTREGISTERED		"451"
# define ERR_NEEDMOREPARAMS		"461"
# define ERR_ALREADYREGISTRED	"462"
# define ERR_PASSWDMISMATCH		"464"

class user;

/*
	serverHandler : server 객체를 통제하는 클래스
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

	void	sendNumericReplies(int user_fd, const std::string& numeric, const std::string& user_name, const std::string& replies);
	void	check_user_info(user* user_, int user_fd);
	void	broadcast_message(int user_fd, const std::string& ch_name, const std::string& cmd);

	bool	commandPass(int user_fd, const std::string& val);
	void	commandNick(int user_fd, const std::string& val);
	void	commandUser(int user_fd, const std::string& val);
	void	commandPrivmsg(int user_fd, const std::string& val);
	void	commandJoin(int user_fd, const std::string& val);
	void	commandPart(int user_fd, const std::string& val);
	void	commandKill(int user_fd, const std::string& val);
	void	commandQuit(int user_fd);
	void	commandAdmin(int user_fd, const std::string& val);
//	void	commandHost(int user_fd, const std::string& val);
	void	commandList(int user_fd);
};

#endif
