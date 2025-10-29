#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp> 
#include "RedisMgr.h"
std::string generate_unique_string() {
	// 创建UUID对象
	boost::uuids::uuid uuid = boost::uuids::random_generator()();

	// 将UUID转换为字符串
	std::string unique_string = boost::uuids::to_string(uuid);

	return unique_string;
}

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
	std::string prefix("llfc status server has received :  ");
	std::lock_guard<std::mutex> lock(_mtx);
	auto& server = _servers.begin()->second;
	reply->set_host(server.host);
	reply->set_port(server.port);
	reply->set_error(ErrorCodes::Success);
	reply->set_token(generate_unique_string());
	insertToken(request->uid(), reply->token());
	return Status::OK;
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, message::LoginRsp* reply)
{
	auto uid = request->uid();
	auto token = request->token();
	std::lock_guard<std::mutex> lock(_mtx);
	auto iter = _tokens.find(uid);
	if(iter==_tokens.end())
	{
		reply->set_error(ErrorCodes::UidInvalid);
		return Status::OK;
	}
	if(iter->second != token)
	{
		reply->set_error(ErrorCodes::TokenInvalid);
		return Status::OK;
	}
	reply->set_error(ErrorCodes::Success);
	reply->set_uid(uid);
	reply->set_token(token);
	return Status::OK;
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
	std::lock_guard<std::mutex> lock(_mtx);
	_tokens[uid] = token;
}



StatusServiceImpl::StatusServiceImpl() :_server_index(0)
{
	auto& cfg = *(ConfigMgr::getInstance());
	ChatServer server;
	server.port = cfg["ChatServer1"]["port"];
	server.host = cfg["ChatServer1"]["host"];
	server.con_count = 0;
	server.name = cfg["ChatServer1"]["name"];
	_servers[server.name]=server;

	server.port = cfg["ChatServer2"]["port"];
	server.host = cfg["ChatServer2"]["host"];
	server.con_count = 0;
	server.name = cfg["ChatServer2"]["name"];
	_servers[server.name] = server;
}