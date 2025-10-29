#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOContextPool.h"

CServer::CServer(boost::asio::io_context& ioc, unsigned short port)
	:_acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _ioc(ioc)
{
}

void CServer::Start() {
	auto self = shared_from_this();
	boost::asio::io_context& io_context = AsioIOContextPool::getInstance()->GetIOContext();
	std::shared_ptr<HttpConnection> new_connection = make_shared<HttpConnection>(io_context);
	_acceptor.async_accept(new_connection->GetSocket(), [self,new_connection](beast::error_code ec) {
		try {
			//出错放弃链接，继续监听其他链接
			if (ec) {
				self->Start();
				return;
			}
			//创建新链接，并且创建HttpConnection类管理这个链接
			new_connection->Start();
			self->Start();
		}
		catch (std::exception& exp) {
			std::cout << "CServer accept error:" << exp.what() << std::endl;
		}
		});
}