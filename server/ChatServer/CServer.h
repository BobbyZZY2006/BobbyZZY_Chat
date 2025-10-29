#pragma once
#include "const.h"
#include "AsioIOContextPool.h"

class CSession;
class CServer
{
public:
	CServer(net::io_context& ioc, short port);
	~CServer();
	bool CheckValid(std::string uuid);
	void CloseSession(const std::string uuid);
	void StartAccept();
	std::shared_ptr<CSession> GetSession(std::string uuid);
private:
	void HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& ec);
	tcp::acceptor _acceptor;
	net::io_context& _ioc;
	void Start();
	std::map<std::string, std::shared_ptr<CSession>> _sessions;
	std::mutex _mutex;
	short _port;
};

