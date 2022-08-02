#ifndef USER_HPP
# define USER_HPP

# include <sys/socket.h>
# include <string>
# include <fcntl.h>

class	user
{
private:
	int					_user_fd;
	struct sockaddr_in	_client_addr;

	std::string	_nick;
	std::string	_user;
	std::string	_host_name;

	bool	_is_pass;
	bool	_is_admin;
	bool	_is_host;
	bool	_sign_in;

	user(void);
	user(const user& other);
	user&	operator=(const user& other);

public:
	user(int user_fd, struct sockaddr_in &client_addr);
	~user(void);

	int					getUserFd(void);
	struct sockaddr_in&	getClientAddr(void);
	std::string&		getNick(void);
	std::string&		getUser(void);
	std::string&		getHostName(void);
	bool				getIsPass(void);
	bool				getIsAdmin(void);
	bool				getIsHost(void);
	bool				getSignIn(void);

	void	setNick(const std::string& nick);
	void	setUser(const std::string& user);
	void	setTruePass(void);
	void	setTrueAdmin(void);
	void	setTrueHost(bool host);
	void	setTrueSignIn(void);
};

#endif
