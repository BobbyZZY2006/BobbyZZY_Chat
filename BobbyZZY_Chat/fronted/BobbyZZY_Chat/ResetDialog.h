#pragma once

#include <QDialog>
#include "ui_ResetDialog.h"
#include <QToolButton>

QT_BEGIN_NAMESPACE
namespace Ui { class ResetDialog; };
QT_END_NAMESPACE

class ResetDialog : public QDialog
{
	Q_OBJECT

public:
	ResetDialog(QWidget *parent = nullptr);
	~ResetDialog();
	QMap<TipErr, QString> _tip_err;
signals:
	void sigSwitchLogin();

private:
	void initHttpHandlers();
	void showTip(QString str, bool b_ok);
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
	Ui::ResetDialog* ui;

private slots:
	void on_get_code_clicked();
	void on_cancel_btn_clicked();
	void on_confirm_btn_clicked();
	void slot_reset_mod_finish(ReqId id, QString res, ErrorCodes err);

};
