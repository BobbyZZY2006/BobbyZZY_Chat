#include "CSession.h"
#include<boost/uuid/uuid.hpp>
#include<boost/uuid/uuid_generators.hpp>
#include<boost/uuid/uuid_io.hpp>
#include "CServer.h"
#include "LogicSystem.h"

CSession::CSession(net::io_context& ioc, CServer* server)
	:_socket(ioc),_b_stop(false)
{
	boost::uuids::uuid uuid = boost::uuids::random_generator()();
	_uuid = boost::uuids::to_string(uuid);
	_headNode = std::make_shared<MsgNode>(MSG_HEAD_LENGTH);
}

CSession::~CSession()
{
	std::cout << "~CSession destruct" << std::endl;
}

tcp::socket& CSession::getSocket()
{
	return _socket;
}

std::string& CSession::getUUID()
{
	return _uuid;
}

void CSession::Start()
{
	readHead();
}

void CSession::readHead() {
	auto self = shared_from_this();
	startNewRead(MSG_HEAD_LENGTH, [self, this](size_t length, const boost::system::error_code& ec) {
		try {
			if (ec) {
				std::cout << "read head error:" << ec.message() << std::endl;
				Stop();
				_server->CloseSession(_uuid);
				return;
			}
			if (length < MSG_HEAD_LENGTH)
			{
				std::cout << "read head length error:" << length << std::endl;
				Stop();
				_server->CloseSession(_uuid);
				return;
			}

			if (!_server->CheckValid(_uuid)) {
				Stop();
				return;
			}

			_headNode->Clear();
			memcpy(_headNode->_data, _data, MSG_HEAD_LENGTH);

			short msgid = 0;
			memcpy(&msgid, _headNode->_data, MSG_ID_LENGTH);
			msgid = ntohs(msgid);
			std::cout << "msgid is " << msgid << std::endl;
			if (msgid > MAX_LENGTH) {
				std::cout << "msgid invalid" << msgid << std::endl;
				_server->CloseSession(_uuid);
				return;
			}

			short msglen = 0;
			memcpy(&msglen, _headNode->_data + MSG_ID_LENGTH, MSG_LENGTH_LENGTH);
			msglen = ntohs(msglen);
			if (msglen > MAX_LENGTH) {
				std::cout << "msglen invalid:" << msglen << std::endl;
				_server->CloseSession(_uuid);
				return;
			}

			_recvNode = std::make_shared<RecvNode>(msglen, msgid);
			readBody(msglen);
		}
		catch (std::exception& e) {
			std::cout << "exception is" << e.what() << std::endl;
			_socket.close();  // 关闭 socket
			_server->CloseSession(_uuid); // 从管理器里移除
		}
	});
}

void CSession::readBody(size_t msglen) {
	auto self = shared_from_this();
	startNewRead(msglen, [self, this,msglen](size_t length, const boost::system::error_code& ec) {
		try {
			if (ec) {
				std::cout << "read head error:" << ec.message() << std::endl;
				Stop();
				_server->CloseSession(_uuid);
				return;
			}
			if (length < msglen)
			{
				std::cout << "read head length error:" << length << std::endl;
				Stop();
				_server->CloseSession(_uuid);
				return;
			}

			if (!_server->CheckValid(_uuid)) {
				Stop();
				return;
			}
			memcpy(_recvNode->_data + MSG_HEAD_LENGTH, _data, msglen);
			_recvNode->_curLength += msglen;
			_recvNode->_data[_recvNode->_totalLength] = '\0';
			std::cout << "receive data is:"<<_recvNode->_data << std::endl;
			//此处将消息投递到逻辑队列中
			LogicSystem::getInstance()->PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), _recvNode));
			readHead();
		}
		catch (std::exception& e) {
			std::cout << "exception is" << e.what() << std::endl;
			_socket.close();  // 关闭 socket
			_server->CloseSession(_uuid); // 从管理器里移除
		}
		});
}


//读取完整长度
void CSession::startNewRead(std::size_t maxLength, handlerType handler)
{
	memset(_data, 0, MAX_LENGTH);
	ReadLength(0, maxLength, handler);
}

void CSession::ReadLength(size_t readedLength, size_t total_length, handlerType handler) {
	auto self = shared_from_this();
	_socket.async_read_some(boost::asio::buffer(_data + readedLength, total_length - readedLength),
		[readedLength, total_length, handler, self](const boost::system::error_code& ec, size_t  bytesTransfered) {
			if (ec) {
				// 出现错误，调用回调函数
				handler(readedLength + bytesTransfered,ec);
				return;
			}

			if (readedLength + bytesTransfered >= total_length) {
				//长度够了就调用回调函数
				handler(readedLength + bytesTransfered,ec);
				return;
			}

			// 没有错误，且长度不足则继续读取
			self->ReadLength(readedLength + bytesTransfered, total_length, handler);
		});
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self) {
	//增加异常处理
	try {
		auto self = shared_from_this();
		if (!error) {
			std::lock_guard<std::mutex> lock(_send_lock);
			//cout << "send data " << _send_que.front()->_data+HEAD_LENGTH << endl;
			_send_que.pop();
			if (!_send_que.empty()) {
				auto& msgnode = _send_que.front();
				boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_totalLength),
					std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_self));
			}
		}
		else {
			std::cout << "handle write failed, error is " << error.what() << std::endl;
			Stop();
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception code : " << e.what() << std::endl;
	}

}

void CSession::Send(const char* msg, short max_length, short msgid) {
	std::lock_guard<std::mutex> lock(_send_lock);
	int send_que_size = _send_que.size();
	if (send_que_size > MAX_SENDQUE) {
		std::cout << "session: " << _uuid << " send que fulled, size is " << MAX_SENDQUE << std::endl;
		return;
	}

	_send_que.push(std::make_shared<SendNode>(msg, max_length, msgid));
	if (send_que_size > 0) {
		return;
	}
	auto& msgnode = _send_que.front();
	boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->_totalLength),
		std::bind(&CSession::HandleWrite, this, std::placeholders::_1, shared_from_this()));
}

void CSession::Send(std::string data, short msgid) {
	Send(data.c_str(), data.length(), msgid);
}

LogicNode::LogicNode(std::shared_ptr<CSession>  session,std::shared_ptr<RecvNode> recvnode) :_session(session), _recvnode(recvnode) {

}

void CSession::Stop() {
	std::lock_guard<std::mutex> lock(_mutex);
	_socket.close();
	_b_stop = true;
}
