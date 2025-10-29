#include "MysqlDao.h"
SqlConnection::SqlConnection(sql::Connection* con, int64_t lasttime)
	:_con(con), _last_oper_time(lasttime) {

}

MySqlPool::MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize)
	: _url(url), _user(user), _pass(pass), _schema(schema), _poolSize(poolSize), _b_stop(false) {
	try {
		for (int i = 0; i < _poolSize; ++i) {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* con = driver->connect(_url, _user, _pass);
			std::cout << _url << " " << _user << " " << _pass << std::endl;
			con->setSchema(_schema);
			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
			_pool.push(std::make_unique<SqlConnection>(con,timestamp));
		}
		_check_thread = std::thread([this]() {
			while (!_b_stop) {
				checkConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));
			}
			});
	}
	catch (sql::SQLException& e) {
		// 处理异常
		std::cout << "mysql pool init failed" << std::endl;
	}
}

void MySqlPool::checkConnection() {
	std::lock_guard<std::mutex> lock(_mutex);
	auto pool_size = _pool.size();
	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	int64_t timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
	for (size_t i = 0; i < pool_size; i++) {
		auto con = std::move(_pool.front());
		_pool.pop();
		Defer defer([this, &con]() {
			_pool.push(std::move(con));
			});
		if (timestamp - con->_last_oper_time < 300) continue;
		try {
			std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
			stmt->execute("SELECT 1");
			con->_last_oper_time = timestamp;
			std::cout << "execute timer alive query" << std::endl;
		}
		catch (sql::SQLException) {
			std::cout << "reconnect to mysql server" << std::endl;
			auto* driver = sql::mysql::get_mysql_driver_instance();
			auto* new_con = driver->connect(_url, _user, _pass);
			new_con->setSchema(_schema);
			con->_con.reset(new_con);
			con->_last_oper_time = timestamp;
		}
	}
}


void MySqlPool::returnConnection(std::unique_ptr<SqlConnection> con) {
	if(con == nullptr) {
		return;
	}
	std::unique_lock<std::mutex> lock(_mutex);
	if (_b_stop) {
		return;
	}
	_pool.push(std::move(con));
	_cv.notify_one();
}

std::unique_ptr<SqlConnection> MySqlPool::getConnection() {
	std::unique_lock<std::mutex> lock(_mutex);
	_cv.wait(lock, [this] {
		if (_b_stop) {
			return true;	
		}
		return !_pool.empty(); });
	if (_b_stop) {
		return nullptr;
	}
	std::unique_ptr<SqlConnection> con(std::move(_pool.front()));
	_pool.pop();
	return con;
}

void MySqlPool::Close() {
	_b_stop = true;
	_cv.notify_all();
}

MySqlPool::~MySqlPool() {
	std::unique_lock<std::mutex> lock(_mutex);
	while (!_pool.empty()) {
		_pool.pop();
	}
}

MysqlDao::MysqlDao() {
	auto &g_config_mgr = *(ConfigMgr::getInstance());
	std::string host = g_config_mgr["Mysql"]["host"];
	std::string port = g_config_mgr["Mysql"]["port"];
	std::string user = g_config_mgr["Mysql"]["user"];
	std::string schema = g_config_mgr["Mysql"]["schema"];
	std::string password = g_config_mgr["Mysql"]["passwd"];
	_pool = std::make_unique<MySqlPool>(host + ":" + port, user, password, schema, 10);
}

MysqlDao::~MysqlDao() {
	_pool->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& password, const std::string& email) {
	auto con = _pool->getConnection();
	try {
		if (con == nullptr || con->_con == nullptr) {
			std::cout << "get mysql connection failed" << std::endl;
			return 0;
		}
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		pstmt->setString(1, name);
		pstmt->setString(2, email);
		pstmt->setString(3, password);
		pstmt->execute();
		std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT @result AS result"));
		if (res->next()) {
			int result = res->getInt("result");
			_pool->returnConnection(std::move(con));
			return result;
		}
		_pool->returnConnection(std::move(con));
		return 0;
	}
	catch (sql::SQLException& e) {
		std::cout << "mysql exception: " << e.what() << std::endl;
		if (con) {
			_pool->returnConnection(std::move(con));
		}
		return 0;
	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = _pool->getConnection();
	try {
		if (con == nullptr) {
			_pool->returnConnection(std::move(con));
			return false;
		}

		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

		// 绑定参数
		pstmt->setString(1, name);

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		// 遍历结果集
		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email")) {
				_pool->returnConnection(std::move(con));
				return false;
			}
			_pool->returnConnection(std::move(con));
			return true;
		}
	}
	catch (sql::SQLException& e) {
		if (con) {
			_pool->returnConnection(std::move(con));
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = _pool->getConnection();
	try {
		if (con == nullptr) {
			_pool->returnConnection(std::move(con));
			return false;
		}

		// 准备查询语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

		// 绑定参数
		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);

		// 执行更新
		int updateCount = pstmt->executeUpdate();

		std::cout << "Updated rows: " << updateCount << std::endl;
		_pool->returnConnection(std::move(con));
		return true;
	}
	catch (sql::SQLException& e) {
		if (con) {
			_pool->returnConnection(std::move(con));
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto con = _pool->getConnection();
	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {
		if (con == nullptr) {
			return false;
		}

		// 准备SQL语句
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE email = ?"));
		pstmt->setString(1, email); // 将username替换为你要查询的用户名

		// 执行查询
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		// 遍历结果集
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			// 输出查询到的密码
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}

		if (pwd != origin_pwd) {
			return false;
		}
		userInfo.user_name = res->getString("name");
		userInfo.email = res->getString("email");
		userInfo.user_id = res->getInt("uid");
		userInfo.password = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}