#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "StatusGrpcClient.h"
void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(std::make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {
	_post_handlers.insert(std::make_pair(url, handler));
}

LogicSystem::LogicSystem() {
	RegGet("/get_test",[](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req\r\n";
		int i = 1;
		for (auto elem : connection->_get_params) {
			beast::ostream(connection->_response.body())
				<< "param" << i++ << " : " << elem.first << "=" << elem.second << "\r\n";
		}
		});

	RegPost("/get_verifycode", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		Json::Value root;
		Json::Reader reader;
		Json::Value request_json;
		bool success = reader.parse(body_str, request_json);
		if (!success || !request_json.isMember("email")) {
			root["code"] = Err_Json;
			root["msg"] = "json parse error";
			beast::ostream(connection->_response.body()) << root.toStyledString();
			return;
		}
		auto email = request_json["email"].asString();
		std::cout << "email is " << email << std::endl;
		
		GetVerifyRsp rsp = VerifyGrpcClient::getInstance()->GetVerifyCode(email);
		root["error"] = rsp.error();
		root["msg"] = "send verify code success";
		root["email"] = email;
		beast::ostream(connection->_response.body()) << root.toStyledString();
		});

    RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Err_Json;
            root["msg"] = "Failed to parse JSON data!";
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return;
        }
		auto email = src_root["email"].asString();
		auto name = src_root["username"].asString();
		auto pwd = src_root["password"].asString();
        //先查找redis中email对应的验证码是否合理
        std::string  verify_code;
        bool b_get_verify = RedisMgr::getInstance()->Get(src_root["email"].asString(), verify_code);
        if (!b_get_verify) {
            std::cout << " get verify code expired" << std::endl;
            root["error"] = ErrorCodes::verifyExpired;
            root["msg"] = " get verify code expired";
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return;
        }

        if (verify_code != CODE_PREFIX+src_root["verify_code"].asString()) {
            std::cout << " verify code error" << std::endl;
            root["error"] = ErrorCodes::verifyCodeErr;
            root["msg"] = " verify code error";
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return;
        }

		int uid = MysqlMgr::getInstance()->RegUser(name, pwd, email);
		if (uid == 0) {
			root["error"] = ErrorCodes::MysqlDaoErr;
			root["msg"] = "register user failed";
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return;
		}
		if (uid == -1) {
			root["error"] = ErrorCodes::MysqlErr;
			root["msg"] = "register user failed";
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return;
		}
		if (uid == -2) {
			root["error"] = ErrorCodes::UserExist;
			root["msg"] = "register user failed";
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return;
		}
		if (uid == -3) {
			root["error"] = ErrorCodes::EmailExist;
			root["msg"] = "register user failed";
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return;
		}

        root["error"] = 0;
        root["email"] = email;
        root["user"] = name;
        root["passwd"] = pwd;
        root["verifycode"] = src_root["verify_code"].asString();
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return;
        });

    //重置回调逻辑
    RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Err_Json;
            root["msg"] = "Failed to parse JSON data!";
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return;
        }

        auto email = src_root["email"].asString();
        auto name = src_root["user"].asString();
        auto pwd = src_root["passwd"].asString();

        //先查找redis中email对应的验证码是否合理
        std::string  verify_code;
        bool b_get_varify = RedisMgr::getInstance()->Get(src_root["email"].asString(), verify_code);
        if (!b_get_varify) {
            std::cout << " get varify code expired" << std::endl;
            root["error"] = ErrorCodes::verifyExpired;
            root["msg"] = " get varify code expired";
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return;
        }

        if (verify_code != CODE_PREFIX + src_root["verify_code"].asString()) {
            std::cout << " verify code error" << std::endl;
            root["error"] = ErrorCodes::verifyCodeErr;
            root["msg"] = " verify code error";
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return;
        }
        //查询数据库判断用户名和邮箱是否匹配
        bool email_valid = MysqlMgr::getInstance()->CheckEmail(name, email);
        if (!email_valid) {
            std::cout << " user email not match" << std::endl;
            root["error"] = ErrorCodes::EmailNotMatch;
            root["msg"] = " user email not match";
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return;
        }

        //更新密码为最新密码
        bool b_up = MysqlMgr::getInstance()->UpdatePwd(name, pwd);
        if (!b_up) {
            std::cout << " update pwd failed" << std::endl;
            root["error"] = ErrorCodes::PasswdUpFailed;
            root["msg"] = " update pwd failed";
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return;
        }

        std::cout << "succeed to update password" << pwd << std::endl;
        root["error"] = 0;
        root["email"] = email;
        root["user"] = name;
        root["passwd"] = pwd;
        root["verifycode"] = src_root["verifycode"].asString();
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
        return;
        });

    RegPost("/user_login", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Err_Json;
            root["msg"] = "Failed to parse JSON data!";
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return;
        }
         
        auto email = src_root["email"].asString();
        auto pwd = src_root["passwd"].asString();
		UserInfo userInfo;
		bool pwd_valid = MysqlMgr::getInstance()->CheckPwd(src_root["email"].asString(), pwd, userInfo);
        if (!pwd_valid) {
			std::cout << "user pwd not match" << std::endl;
            root["error"] = ErrorCodes::PasswdInvalid;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
            return;
        }
		auto reply = StatusGrpcClient::getInstance()->GetChatServer(userInfo.user_id);
        if (reply.error()) {
            std::cout << "rpc get chat server failed,error is" << reply.error() << std::endl;
			root["error"] = reply.error();
			std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
			return;
        }

		std::cout << "succed to load userinfo uid is" << userInfo.user_id << std::endl;
		root["error"] = 0;
        root["email"] = email;
		root["uid"] = userInfo.user_id;
		root["token"] = reply.token();
		root["host"] = reply.host();
		root["port"] = reply.port();
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
        return;
        });
}

bool LogicSystem::HandleGet(std::string url, std::shared_ptr<HttpConnection> connection) {
	auto it = _get_handlers.find(url);
	if (it != _get_handlers.end()) {
		it->second(connection);
		return true;
	}
	return false;
}

bool LogicSystem::HandlePost(std::string url, std::shared_ptr<HttpConnection> connection) {
	auto it = _post_handlers.find(url);
	if (it != _post_handlers.end()) {
		it->second(connection);
		return true;
	}
	return false;
}

LogicSystem::~LogicSystem() {
}