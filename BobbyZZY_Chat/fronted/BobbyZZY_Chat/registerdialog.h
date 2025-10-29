#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QToolButton>
#include "global.h"
#include <QTimer>

namespace Ui {
class RegisterDialog;
}

class RegisterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = nullptr);
    ~RegisterDialog();
	QMap<TipErr, QString> _tip_err;
signals:
	void sigSwitchLogin();

private:
	void initHttpHandlers();
    void showTip(QString str,bool b_ok);
    Ui::RegisterDialog *ui;
	QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;
    void checkPwdValid();
	void checkUserValid();
	void checkEmailValid();
	void checkConfirmValid();
	void checkVerifyValid();
	void addTipErr(TipErr err, QString tip);
	void removeTipErr(TipErr err);
	void ChangeTipPage();
	QTimer* _countdown_timer;
	int _countdown;
private slots:
    void on_get_code_clicked();
	void on_return_btn_clicked();
	void on_cancel_btn_clicked();
	void on_confirm_btn_clicked();
	void slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err);
};

#endif // REGISTERDIALOG_H
