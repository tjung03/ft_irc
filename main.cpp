#include <iostream>
#include <csignal>
#include "main.hpp"

bool	g_server_on = true;

void	sig_handler(int sig)
{
	if (sig == SIGINT)
		g_server_on = false;
}

/*
	./ircserv <port> <server_password> 입력 받음

	signal	- SIGINT	: ctrl + c 입력의 경우 발생
			- SIGPIPE	: 비정상적 종료된 클라이언트에게 write 하게 되었을 때 발생

	severHandler 클래스를 통해 server 컨트롤 함.
*/
int	main(int argc, char *argv)
{
	if (argc != 3)
	{
		std::cout<<INVALID_ARGU_CNT<<"\n./ircserv <port> <server_password>"<<std::endl;
		return (1);
	}

	signal(SIGINT, sig_handler);
	// SIGPIPE 신호를 무시하여 서버가 강제 종료되는 것을 방지
	signal(SIGPIPE, SIG_IGN);

	serverHandler	handler(av);

	while (g_server_on)
		handler.server_on();

	return (0);
}
