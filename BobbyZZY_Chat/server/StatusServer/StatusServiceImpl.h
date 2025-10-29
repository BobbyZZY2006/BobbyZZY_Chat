#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;
using message::LoginReq;

struct ChatServer {
	std::string host;
	std::string port;
	int con_count;
	std::string name;
};

struct ChatServerCmp {
	bool operator()(const ChatServer& s1, const ChatServer& s2) const {
		return s1.con_count < s2.con_count;
	}
};

class StatusServiceImpl final : public StatusService::Service
{
public:
	StatusServiceImpl();
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
		GetChatServerRsp* reply) override;
	Status Login(ServerContext* context, const LoginReq* request,
		message::LoginRsp* reply) override;
private:
	void insertToken(int uid, std::string token);
	std::mutex _mtx;
	std::map < std::string, ChatServer, ChatServerCmp> _servers;
	std::map<int, std::string> _tokens;
	int _server_index;
};
