#include "TcpMgr.h"
#include "UserMgr.h"
TcpMgr::TcpMgr():_b_pending(false),_socket(),_id(0),_host(""),_port(0),_length(0)
{
	connect(&_socket, &QTcpSocket::connected, [&] {
		qDebug() << "successfully connect to the server" << Qt::endl;
		emit sig_tcp_con_res(true);
		});
	connect(&_socket, &QTcpSocket::readyRead, [&] {
		_buffer.append(_socket.readAll());
		QDataStream stream(_buffer);
		forever{
			if (!_b_pending) {
				if (_buffer.size() < sizeof(uint16_t) * 2) return;
				stream >> _id >> _length;
				_buffer.mid(sizeof(uint16_t) * 2);
				qDebug() << "messsage id:" << _id << " message length:" << _length;
			}
			if (_buffer.size() < _length) {
				_b_pending = true;
				return;
			}
			QByteArray message = _buffer.mid(0, _length);
			_b_pending = false;
			qDebug() << "message is" << message;
			_buffer = _buffer.mid(_length);
			handleMsg(ReqId(_id), _length,message);
		}
	});
	connect(&_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred), [&](QAbstractSocket::SocketError socketError) {
		Q_UNUSED(socketError)
		qDebug() << "Error:" << _socket.errorString();
		emit sig_tcp_con_res(false);
       });
	connect(&_socket, &QTcpSocket::disconnected, [&]() {
		qDebug() << "Disconnected from server.";
		});
	connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);
}

void TcpMgr::handleMsg(ReqId reqId, uint16_t length, QByteArray data)
{
	auto find_iter = _handlers.find(reqId);
	if (find_iter != _handlers.end()) {
		find_iter.value()(reqId, length, data);
	}
	else {
		qDebug() << "no handler for reqId" << reqId;
	}
}

void TcpMgr::initHandlers()
{
	_handlers.insert(ID_CHAT_LOGIN_RSP, [this](ReqId id, int16_t len, QByteArray data) {
		Q_UNUSED(len);
		qDebug() << "handle id is" << id << " data is " << data;
		QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
		if (jsonDoc.isNull() || !jsonDoc.isObject()) {
			qDebug() << "json parse error";
			return;
		}
		QJsonObject obj = jsonDoc.object();
		if (!obj.contains("error")) {
			qDebug() << "Login Failed,err is Json Parse Err" << ErrorCodes::Err_Json;
			emit sig_login_failed(ErrorCodes::Err_Json);
		}
		if (obj["error"] != ErrorCodes::SUCCESS) {
			int err = obj["error"].toInt();
			qDebug() << "Login Failed,err is" << obj["error"].toInt();
			emit sig_login_failed(obj["error"].toInt());
			return;
		}

		UserMgr::getInstance()->setUid(obj["uid"].toInt());
		UserMgr::getInstance()->setName(obj["name"].toString());
		UserMgr::getInstance()->setToken(obj["token"].toString());
		emit sig_switch_chatdlg();
		});
}

void TcpMgr::slot_tcp_connect(ServerInfo si) {
	qDebug() << "receive tcp connect signal";
	// 尝试连接到服务器
	qDebug() << "Connecting to server...";
	_host = si.host;
	_port = static_cast<uint16_t>(si.port.toUInt());
	_socket.connectToHost(_host, _port);
}

void TcpMgr::slot_send_data(ReqId reqId, QString data) {
	uint16_t uid = reqId;
	QByteArray dataBytes = data.toUtf8();
	uint16_t length = static_cast<uint16_t>(dataBytes.length());
	QByteArray send;
	QDataStream out(&send, QDataStream::WriteOnly);
	out.setByteOrder(QDataStream::BigEndian);
	out << uid << length;
	send.append(dataBytes);
	_socket.write(send);
}

