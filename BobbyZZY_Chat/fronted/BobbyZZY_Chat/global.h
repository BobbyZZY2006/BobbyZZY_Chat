#pragma once
#pragma execution_character_set("utf-8")
#include<qwidget.h>
#include<functional>
#include<qregularexpression.h>
#include "Qstyle"
#include<memory>
#include<iostream>
#include<mutex>
#include<QByteArray>
#include<QNetWorkReply>
#include<QJsonObject>
#include<QSettings>
#include<QDir>
#include <QMap>

//repolish用来刷新css
extern std::function<void(QWidget*)> repolish;
extern std::function<QString(QString)> xorString;

enum ReqId {
	ID_GET_verify_CODE=1001,
	ID_REG_USER=1002,
	ID_RESET_PWD = 1003,
	ID_LOGIN_USER = 1004,
	ID_CHAT_LOGIN=1005,
	ID_CHAT_LOGIN_RSP = 1006,
};

enum Modules {
	REGISTERMOD=0,
	RESETMOD = 1,
	LOGINMOD = 2,
};

enum ErrorCodes {
	SUCCESS = 0,
	ERR_NETWORK = 1,
	Err_Json = 1001,
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

enum TipErr {
	TIP_USER_ERR = 1,
	TIP_EMAIL_ERR = 2,
	TIP_PWD_ERR = 3,
	TIP_CONFIRM_ERR = 4,
	TIP_VERIFY_ERR = 5,
};
extern QString gate_url_prefix;

struct ServerInfo {
	QString host;
	QString port;
	QString token;
	int uid;
};