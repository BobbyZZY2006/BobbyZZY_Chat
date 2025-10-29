#include "VerifyGrpcClient.h"
RPConPool::RPConPool(size_t poolSize, std::string host, std::string port)
	:_poolSize(poolSize), _host(host), _port(port), _b_stop(false)
{
	for (size_t i = 0; i < _poolSize; i++) {
		std::shared_ptr<Channel> channel = grpc::CreateChannel(
			_host + ":" + _port, grpc::InsecureChannelCredentials());
		std::unique_ptr<VerifyService::Stub> stub = VerifyService::NewStub(channel);
		_connections.push(std::move(stub));
	}
}

RPConPool::~RPConPool() {
	std::lock_guard<std::mutex> lock(_mtx);
	Close();
	while(!(_connections.empty())) {
		_connections.pop();
	}
}

void RPConPool::Close() {
	_b_stop = true;
	_cv.notify_all();
}

std::unique_ptr<VerifyService::Stub> RPConPool::GetConnection() {
	std::unique_lock<std::mutex> lock(_mtx);
	_cv.wait(lock, [this]() {return !_connections.empty() || _b_stop; });
	if (_b_stop) {
		return nullptr;
	}
	auto conn = std::move(_connections.front());
	_connections.pop();
	return conn;
}

void RPConPool::returnConnection(std::unique_ptr<VerifyService::Stub> context) {
	std::lock_guard<std::mutex> lock(_mtx);
	if (_b_stop) return;
	_connections.push(std::move(context));
	_cv.notify_one();
}

GetVerifyRsp VerifyGrpcClient::GetVerifyCode(std::string email) {
	ClientContext context;
	GetVerifyReq request;
	GetVerifyRsp response;
	request.set_email(email);
	auto stub = _pool->GetConnection();
	Status status = stub->GetVerifyCode(&context, request, &response);
	if (status.ok()) {
		_pool->returnConnection(std::move(stub));
		return response;
	}
	else {
		_pool->returnConnection(std::move(stub));
		response.set_error(ErrorCodes::Err_RPC);
		return response;
	}
}

VerifyGrpcClient::VerifyGrpcClient() {
	auto &g_config_mgr = *(ConfigMgr::getInstance());
	std::string host = g_config_mgr["VerifyServer"]["host"];
	std::string port = g_config_mgr["VerifyServer"]["port"];
	_pool.reset(new RPConPool(std::thread::hardware_concurrency(), host, port));

}