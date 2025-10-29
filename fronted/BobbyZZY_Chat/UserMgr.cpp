#include "UserMgr.h"

void UserMgr::setName(QString name)
{
	_name = name;
}

void UserMgr::setUid(int uid)
{
	_uid = uid;
}
void UserMgr::setToken(QString token)
{
	_token = token;
}
UserMgr::UserMgr()
{
}
UserMgr::~UserMgr()
{
}