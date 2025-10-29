#include "CServer.h"
#include "CSession.h"

CServer::CServer(net::io_context& ioc, short port) :_ioc(ioc), _port(port),
_acceptor(ioc, tcp::endpoint(tcp::v4(), port))
{
	std::cout << "Server start success, listen on port : " << _port << std::endl;

	StartAccept();
}

void CServer::StartAccept() {
	auto& io_context = AsioIOContextPool::getInstance()->GetIOContext();
	std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);
	_acceptor.async_accept(new_session->getSocket(), std::bind(&CServer::HandleAccept, this, new_session, std::placeholders::_1));
}

CServer::~CServer() {

}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& error) {
	if (!error) {
		new_session->Start();
		std::lock_guard<std::mutex> lock(_mutex);
		_sessions.insert(make_pair(new_session->getUUID(), new_session));
	}
	else {
		std::cout << "session accept failed, error is " << error.what() << std::endl;
	}

	StartAccept();
}

void CServer::CloseSession(std::string session_id) {

	std::lock_guard<std::mutex> lock(_mutex);
	if (_sessions.find(session_id) != _sessions.end()) {
		auto uid = _sessions[session_id]->getUUID();

	}

	_sessions.erase(session_id);

}

std::shared_ptr<CSession> CServer::GetSession(std::string uuid) {
	std::lock_guard<std::mutex> lock(_mutex);
	auto it = _sessions.find(uuid);
	if (it != _sessions.end()) {
		return it->second;
	}
	return nullptr;
}

bool CServer::CheckValid(std::string uuid)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto it = _sessions.find(uuid);
	if (it != _sessions.end()) {
		return true;
	}
	return false;
}
