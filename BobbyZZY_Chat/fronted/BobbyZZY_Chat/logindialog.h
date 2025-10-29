#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "global.h"
#include <QToolButton>

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();

private:
    Ui::LoginDialog *ui;
    virtual void resizeEvent(QResizeEvent* event) override {
        QDialog::resizeEvent(event);
        initUI();
	}
	void initUI();
	void showTip(QString str, bool b_ok);
	bool checkUserValid();
	bool checkPwdValid();
	void initHttpHandlers();
	QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
	ServerInfo _si;
signals:
    void switchRegister();
	void switchReset();
	void sig_connect_tcp(ServerInfo);

public slots:
	void slot_forget_pwd();
	void slot_login_mod_finish(ReqId id, QString res, ErrorCodes err);
	void on_login_btn_clicked();
	void slot_tcp_con_res(bool);
	void slot_login_failed(int);
};

#endif // LOGINDIALOG_H
