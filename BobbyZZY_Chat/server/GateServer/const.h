#pragma once

#include <boost/beast/http.hpp>
#include<boost/beast.hpp>
#include <boost/asio.hpp>
#include<iostream>
#include"Singleton.h"
#include <functional>
#include <map>
#include <unordered_map>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include<boost/filesystem.hpp>
#include<boost/property_tree/ptree.hpp>
#include<boost/property_tree/ini_parser.hpp>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <hiredis/hiredis.h>
#include<cassert>
#include "ConfigMgr.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

enum ErrorCodes{
	Success=0,
	Err_Json=1001,
	Err_RPC = 1002,
	verifyExpired = 1003,
	verifyCodeErr = 1004,
	PasswordErr = 1005,
	EmailNotMatch = 1006,
	PasswdUpFailed = 1007,
	PasswdInvalid = 1008,
	UserExist = 1009,
	EmailExist = 1010,
	MysqlDaoErr = 1011,
	MysqlErr = 1012,
};


#define CODE_PREFIX "code_"

class Defer {
public:
	Defer(std::function<void()> func) :_func(func) {}
	~Defer() {
		if (_func) _func();
	}
private:
	std::function<void()> _func;
};


