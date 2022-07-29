#ifndef SERVERHANDLER_HPP
# define SERVERHANDLER_HPP

# include "server.hpp"

/*
    serverHandler : server 객체를 통제하는 목적
*/
class	serverHandler
{
private:
    server  _serv;

    serverHandler(void);
	serverHandler(const serverHandler& other);
	serverHandler&	operater=(const serverHandler& other);

public:
    serverHandler(char* av[]);
    ~serverHandler(void);

    void    server_on(void);
};

#endif
