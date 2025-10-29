#pragma once
#include "Singleton.h"
#include "global.h"

class TcpMgr:public QObject, public Singleton<TcpMgr>
{
	friend class Singleton<TcpMgr>;
	Q_OBJECT

private:
	TcpMgr();
	void handleMsg(ReqId reqId, uint16_t length, QByteArray data);
	void initHandlers();
	QByteArray _buffer;
	QString _host;
	uint16_t _port;
	QTcpSocket _socket;
	bool _b_pending;
	uint16_t _id, _length;
	QMap<ReqId, std::function<void(ReqId id,int16_t len,QByteArray data)>> _handlers;

public slots:
	void slot_tcp_connect(ServerInfo);
	void slot_send_data(ReqId reqId, QString data);

signals:
	void sig_send_data(ReqId reqId, QString data);
	void sig_tcp_con_res(bool);
	void sig_login_failed(int);
	void sig_switch_chatdlg();
};

