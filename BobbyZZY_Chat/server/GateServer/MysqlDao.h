#pragma once
#include "const.h"
#include<jdbc/mysql_driver.h>
#include<jdbc/mysql_connection.h>
#include<jdbc/cppconn/statement.h>
#include<jdbc/cppconn/prepared_statement.h>
#include<jdbc/cppconn/resultset.h>
#include<jdbc/cppconn/exception.h>

class SqlConnection {
public:
	SqlConnection(sql::Connection* con, int64_t lasttime);
	std::unique_ptr<sql::Connection> _con;
	int64_t _last_oper_time;
};
class MySqlPool {
public:
	MySqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize);
	std::unique_ptr<SqlConnection> getConnection();

	void returnConnection(std::unique_ptr<SqlConnection> con);
	void checkConnection();
	void Close();
	~MySqlPool();

private:
	std::string _url;
	std::string _user;
	std::string _pass;
	std::string _schema;
	int _poolSize;
	std::queue<std::unique_ptr<SqlConnection>> _pool;
	std::mutex _mutex;
	std::condition_variable _cv;
	std::atomic<bool> _b_stop;
	std::thread _check_thread;
};

struct UserInfo {
	int64_t user_id;
	std::string user_name;
	std::string password;
	std::string email;
};

class MysqlDao {
public:
	MysqlDao();
	~MysqlDao();
	int RegUser(const std::string& name, const std::string& password, const std::string& email);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& newpwd);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);

private:
	std::unique_ptr<MySqlPool> _pool;
};