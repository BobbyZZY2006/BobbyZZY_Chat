#pragma once
#include "Singleton.h"
#include "MysqlDao.h"
class MysqlMgr :public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();
	int RegUser(const std::string& name, const std::string& password, const std::string& email);
	bool CheckEmail(const std::string& name, const std::string& email);
	bool UpdatePwd(const std::string& name, const std::string& pwd);
	bool CheckPwd(const std::string& name, const std::string& pwd,UserInfo& userInfo);
private:
	MysqlMgr();
	MysqlDao _dao;
};

