#pragma once
#include "const.h"
#include "MsgNode.h"
using handlerType=std::function<void(size_t, const boost::system::error_code&)>;
class CServer;
class CSession :public std::enable_shared_from_this<CSession>
{
public:
	CSession(net::io_context&, CServer*);
	~CSession();
	tcp::socket& getSocket();
	std::string& getUUID();
	void readHead();
	void readBody(std::size_t);
	void startNewRead(std::size_t length, handlerType);
	void ReadLength(std::size_t readed,std::size_t totalLen,handlerType);
	void Start();
	void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);
	void Send(const char* data, short length, short msgid);
	void Send(std::string data, short msgid);
	void Stop();
private:
	tcp::socket _socket;
	std::string _uuid;
	char _data[MAX_LENGTH];
	bool _b_stop;
	std::shared_ptr<RecvNode> _recvNode;
	std::shared_ptr<MsgNode> _headNode;
	std::shared_ptr<SendNode> _sendNode;
	std::mutex _send_lock;
	std::queue<std::shared_ptr<SendNode>> _send_que;
	CServer* _server;
	std::mutex _mutex;

};
class LogicNode {
	friend class LogicSystem;
public:
	LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);
private:
	std::shared_ptr<CSession> _session;
	std::shared_ptr<RecvNode> _recvnode;
};