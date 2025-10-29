#pragma once 
#include "global.h"
#include "Singleton.h"
class UserMgr:public QObject, public Singleton<UserMgr>
	, public std::enable_shared_from_this<UserMgr>
{
	friend class Singleton<UserMgr>;
	Q_OBJECT
public:
	~UserMgr();
	void setName(QString name);
	void setUid(int uid);
	void setToken(QString token);
private:
	UserMgr();
	QString _name;
	int _uid;
	QString _token;
};

