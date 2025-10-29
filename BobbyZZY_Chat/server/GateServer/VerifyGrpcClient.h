#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVerifyReq;
using message::GetVerifyRsp;
using message::VerifyService;

class RPConPool {
public:
	RPConPool(size_t poolSize, std::string host, std::string port);
	~RPConPool();
	void Close();
	std::unique_ptr<VerifyService::Stub> GetConnection();
	void returnConnection(std::unique_ptr<VerifyService::Stub> context);

private:
	std::string _host;
	std::string _port;
	size_t _poolSize;
	std::atomic<bool> _b_stop;
	std::queue<std::unique_ptr<VerifyService::Stub>> _connections;
	std::mutex _mtx;
	std::condition_variable _cv;
};
class VerifyGrpcClient:public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	GetVerifyRsp GetVerifyCode(std::string email);
private:
	VerifyGrpcClient();
	std::unique_ptr<RPConPool> _pool;
};

