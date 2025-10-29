#include "StatusGrpcClient.h"

StatusConPool::StatusConPool(std::size_t poolSize, std::string host, std::string port):
	_poolSize(poolSize), _host(host), _port(port), _b_stop(false) {
	for(std::size_t i = 0; i < _poolSize; ++i) {
		std::string target_str = _host + ":" + _port;
		auto channel = grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials());
		auto stub = StatusService::NewStub(channel);
		_connections.push(std::move(stub));
	}
}

StatusConPool::~StatusConPool() {
	std::lock_guard<std::mutex> lock(_mtx);
	Close();
	while(!_connections.empty()) {
		_connections.pop();
	}
}

std::unique_ptr<StatusService::Stub> StatusConPool::GetConnection() {
	std::unique_lock<std::mutex> lock(_mtx);
	_cv.wait(lock, [this]{
		if(_b_stop) {
			return true;	
		}
		return !_connections.empty();});
	if(_b_stop) {
		return nullptr;
	}
	auto con = std::move(_connections.front());
	_connections.pop();
	return con;
}

void StatusConPool::returnConnection(std::unique_ptr<StatusService::Stub> con) {
	std::lock_guard<std::mutex> lock(_mtx);
	if(_b_stop) {
		return;
	}
	_connections.push(std::move(con));
	_cv.notify_one();
}

void StatusConPool::Close() {
	_b_stop = true;
	_cv.notify_all();
}

StatusGrpcClient::~StatusGrpcClient()
{
	auto& g_config_mgr = *(ConfigMgr::getInstance());
	std::string host = g_config_mgr["StatusServer"]["host"];
	std::string port = g_config_mgr["StatusServer"]["port"];
	_pool.reset(new StatusConPool(std::thread::hardware_concurrency(), host, port));
}

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid) {
	ClientContext context;
	GetChatServerRsp rsp;
	GetChatServerReq req;
	req.set_uid(uid);
	auto stub = _pool->GetConnection();
	Status status = stub->GetChatServer(&context, req, &rsp);
	Defer defer([this, &stub]() {
		_pool->returnConnection(std::move(stub));
		});
	if(status.ok()) {
		return rsp;
	} else {
		rsp.set_error(ErrorCodes::Err_RPC);
		return rsp;
	}
}

LoginRsp StatusGrpcClient::Login(int uid, std::string token)
{
	ClientContext context;
	LoginRsp reply;
	LoginReq request;
	request.set_uid(uid);
	request.set_token(token);

	auto stub = _pool->GetConnection();
	Status status = stub->Login(&context, request, &reply);
	Defer defer([&stub, this]() {
		_pool->returnConnection(std::move(stub));
		});
	if (status.ok()) {
		return reply;
	}
	else {
		reply.set_error(ErrorCodes::Err_RPC);
		return reply;
	}
}


StatusGrpcClient::StatusGrpcClient()
{
	auto& gCfgMgr = *(ConfigMgr::getInstance());
	std::string host = gCfgMgr["StatusServer"]["host"];
	std::string port = gCfgMgr["StatusServer"]["port"];
	_pool.reset(new StatusConPool(5, host, port));
}
