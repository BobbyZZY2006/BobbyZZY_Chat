#include "HttpConnection.h"
#include "LogicSystem.h"
HttpConnection::HttpConnection(boost::asio::io_context& ioc):
	_socket(ioc)
{
}

void HttpConnection::Start() {
	auto self = shared_from_this();
	http::async_read(_socket, _buffer, _request,
		[self](beast::error_code ec, std::size_t bytes_transferred) {
			try {
				if (ec) {
					std::cout << "http read error is" << ec.what() << std::endl;
					return;
				}
				boost::ignore_unused(bytes_transferred);
				self->HandleRequest();
				self->CheckDeadline();
			}
			catch (std::exception& exp) {
				std::cout << "exception is" << exp.what() << std::endl;
			}
		});
}

void HttpConnection::HandleRequest() {
	_response.version(_request.version());
	_response.keep_alive(false);
	if (_request.method() == http::verb::get) {
		PreParseGetParam();
		bool success=
			LogicSystem::getInstance()->HandleGet(_get_url, shared_from_this());
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body())<< "url not found\r\n";
			WriteResponse();
			return;
		}
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
	if(_request.method() == http::verb::post) {
		bool success=
			LogicSystem::getInstance()->HandlePost(_request.target(), shared_from_this());
		if (!success) {
			_response.result(http::status::not_found);
			_response.set(http::field::content_type, "text/plain");
			beast::ostream(_response.body()) << "url not found\r\n";
			WriteResponse();
			return;
		}
		_response.result(http::status::ok);
		_response.set(http::field::server, "GateServer");
		WriteResponse();
		return;
	}
}

void HttpConnection::WriteResponse() {
	auto self = shared_from_this();
	_response.content_length(_response.body().size());
	http::async_write(_socket, _response,
		[self](beast::error_code ec, std::size_t bytes_transferred) {
			self->_socket.shutdown(tcp::socket::shutdown_send, ec);
			self->_deadline.cancel();
		});
}

void HttpConnection::CheckDeadline() {
	auto self = shared_from_this();
	_deadline.async_wait([self](beast::error_code ec) {
		if (!ec) {
			self->_socket.close(ec);
		}
	});
}

unsigned char ToHex(unsigned char x)
{
	return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
	unsigned char y;
	if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
	else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
	else if (x >= '0' && x <= '9') y = x - '0';
	else assert(0);
	return y;
}

std::string UrlEncode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//判断是否仅有数字和字母构成
		if (isalnum((unsigned char)str[i]) ||
			(str[i] == '-') ||
			(str[i] == '_') ||
			(str[i] == '.') ||
			(str[i] == '~'))
			strTemp += str[i];
		else if (str[i] == ' ') //为空字符
			strTemp += "+";
		else
		{
			//其他字符需要提前加%并且高四位和低四位分别转为16进制
			strTemp += '%';
			strTemp += ToHex((unsigned char)str[i] >> 4);
			strTemp += ToHex((unsigned char)str[i] & 0x0F);
		}
	}
	return strTemp;
}

std::string UrlDecode(const std::string& str)
{
	std::string strTemp = "";
	size_t length = str.length();
	for (size_t i = 0; i < length; i++)
	{
		//还原+为空
		if (str[i] == '+') strTemp += ' ';
		//遇到%将后面的两个字符从16进制转为char再拼接
		else if (str[i] == '%')
		{
			assert(i + 2 < length);
			unsigned char high = FromHex((unsigned char)str[++i]);
			unsigned char low = FromHex((unsigned char)str[++i]);
			strTemp += high * 16 + low;
		}
		else strTemp += str[i];
	}
	return strTemp;
}

void HttpConnection::PreParseGetParam() {
	auto uri = _request.target();
	auto pos = uri.find("?");
	if (pos == std::string::npos) {
		_get_url = uri;
		return;
	}
	_get_url = uri.substr(0, pos);
	std::string query_string = uri.substr(pos + 1);
	std::string key, value;
	size_t position = 0;
	while ((position = query_string.find("&")) != std::string::npos) {
		auto param = query_string.substr(0, position);
		auto pos_equal = param.find("=");
		if (pos_equal != std::string::npos) {
			key = param.substr(0, pos_equal);
			value = param.substr(pos_equal + 1);
			_get_params.insert(std::make_pair(key, UrlDecode(value)));
		}
		query_string.erase(0, position + 1);
	}

	if (!query_string.empty()) {
		auto pos_equal = query_string.find("=");
		if (pos_equal != std::string::npos) {
			key = query_string.substr(0, pos_equal);
			value = query_string.substr(pos_equal + 1);
			_get_params.insert(std::make_pair(key, UrlDecode(value)));
		}
	}
}