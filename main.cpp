#include <iostream>
#include <csignal>
#include "main.hpp"

/*
	To do list (serverOn)
	1. 유저 등록
	2. 메세지 파싱
	3. cmd_part
	4. numeric replies
*/

bool	g_server_on = true;

void	sig_handler(int sig)
{
	if (sig == SIGINT)
		g_server_on = false;
}

/*
	./ircserv <port> <server_password> 입력 받음 (기본 포트 6667, 6668)

	signal	- SIGINT	: ctrl + c 입력의 경우 발생
			- SIGPIPE	: 일방적으로 종료된 클라이언트 소켓에 write 하게 되었을 때 발생

	severHandler 클래스를 통해 server 컨트롤 함.
*/
int	main(int argc, char *argv[])
{
	if (argc != 3)
	{
		std::cerr<<INVALID_ARGU_CNT<<"\n./ircserv <port> <server_password>"<<std::endl;
		return (1);
	}

	signal(SIGINT, sig_handler);
	// SIGPIPE 신호를 무시하여 서버가 강제 종료되는 것을 방지
	signal(SIGPIPE, SIG_IGN);

	serverHandler	handler(argv);

	while (g_server_on)
		handler.serverOn();

	return (0);
}
