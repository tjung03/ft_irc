#include "serverHandler.hpp"

serverHandler::serverHandler(char* av[])
	: _serv(av)
{
	std::cout<<"* launch server *"<<std::endl;
}

serverHandler::~serverHandler(void)
{
	std::cout<<"* finish server *"<<std::endl;
}

void	serverHandler::server_on(void)
{
	//this->_serv
}
