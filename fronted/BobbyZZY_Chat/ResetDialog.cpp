#include "ResetDialog.h"
#include "HttpMgr.h"
#include "global.h"

ResetDialog::ResetDialog(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::ResetDialog())
{
	ui->setupUi(this);
	ui->pass_edit->setEchoMode(QLineEdit::Password);
	ui->err_tip->clear();
	ui->err_tip->setProperty("state", "ok");
	repolish(ui->err_tip);

	QToolButton* passwd_visible_btn = new QToolButton(ui->pass_edit);
	passwd_visible_btn->setObjectName("passwd_visible_btn");
	passwd_visible_btn->setCursor(Qt::PointingHandCursor);
	passwd_visible_btn->setCheckable(true);
	ui->pass_edit->setTextMargins(0, 0, passwd_visible_btn->sizeHint().width(), 0);
	connect(passwd_visible_btn, &QToolButton::toggled, this, [this](bool checked) {
		if (checked)  ui->pass_edit->setEchoMode(QLineEdit::Normal);
		else  ui->pass_edit->setEchoMode(QLineEdit::Password);
		qDebug() << "passwd_visible_btn checked =" << checked;
		});
	// 用布局把按钮固定在右边
	auto layout = new QHBoxLayout(ui->pass_edit);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addStretch();
	layout->addWidget(passwd_visible_btn);
    connect(ui->user_edit, &QLineEdit::editingFinished, this, [this]() {
        checkUserValid();
        });

    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this]() {
        checkEmailValid();
        });

    connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this]() {
        checkPwdValid();
        });


    connect(ui->verify_edit, &QLineEdit::editingFinished, this, [this]() {
        checkVerifyValid();	
        });

    //连接reset相关信号和注册处理回调
	initHttpHandlers();
    connect(HttpMgr::getInstance().get(), &HttpMgr::sig_reset_mod_finish, this,
        &ResetDialog::slot_reset_mod_finish);
}

ResetDialog::~ResetDialog()
{
	delete ui;
}

void ResetDialog::on_cancel_btn_clicked()
{
	emit sigSwitchLogin();
}

void ResetDialog::addTipErr(TipErr err, QString tip) {
	_tip_err[err] = tip;
	showTip(tip, false);
}

void ResetDialog::removeTipErr(TipErr err) {
	_tip_err.remove(err);
	if (_tip_err.empty()) {
		ui->err_tip->clear();
	}
	else {
		showTip(_tip_err.begin().value(), false);
	}
}

void ResetDialog::checkPwdValid() {
	auto pwd = ui->pass_edit->text();
	if (pwd.length() < 6 || pwd.length() > 15) {
		//提示长度不准确
		addTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
		return;
	}
	QRegularExpression pwd_regex("^[a-zA-Z0-9!@#$%^&*]{6,15}$");
	bool match = pwd_regex.match(pwd).hasMatch();
	if (!match) {
		addTipErr(TipErr::TIP_PWD_ERR, tr("密码中不能包含非法字符"));
		return;
	}
	removeTipErr(TipErr::TIP_PWD_ERR);
}

void ResetDialog::checkUserValid() {
	auto user = ui->user_edit->text();
	if (user.length() < 3 || user.length() > 15) {
		//提示长度不准确
		addTipErr(TipErr::TIP_USER_ERR, tr("用户名长度应为3~15"));
		return;
	}
	QRegularExpression user_regex("^[a-zA-Z0-9!@#$%^&*]{3,15}$");
	bool match = user_regex.match(user).hasMatch();
	if (!match) {
		addTipErr(TipErr::TIP_USER_ERR, tr("用户名中不能包含非法字符"));
		return;
	}
	removeTipErr(TipErr::TIP_USER_ERR);
}

void ResetDialog::checkEmailValid() {
	auto email = ui->email_edit->text();
	QRegularExpression email_regex(R"([A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,})");
	bool match = email_regex.match(email).hasMatch();
	if (!match) {
		addTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱格式错误"));
		return;
	}
	removeTipErr(TipErr::TIP_EMAIL_ERR);
}


void ResetDialog::checkVerifyValid() {
	auto verify = ui->verify_edit->text();
	if (verify.length() != 4) {
		addTipErr(TipErr::TIP_VERIFY_ERR, tr("验证码长度应为4"));
		return;
	}
	removeTipErr(TipErr::TIP_VERIFY_ERR);
}


void ResetDialog::on_get_code_clicked() {
	qDebug() << "receive verify btn clicked ";
	if (!ui->email_edit->text().length()) showTip(tr("邮箱不能为空！"), false);
	else if (_tip_err.empty()) {
		ui->get_code->changeEvent();
		QJsonObject json_obj;
		json_obj["email"] = ui->email_edit->text();
		HttpMgr::getInstance()->PostHttpReq(QUrl(gate_url_prefix + "/get_verifycode"), json_obj, ReqId::ID_GET_verify_CODE, Modules::RESETMOD);
	}

}

void ResetDialog::slot_reset_mod_finish(ReqId id, QString res, ErrorCodes err) {
	if (err != ErrorCodes::SUCCESS) {
		if (err == ErrorCodes::ERR_NETWORK) showTip(tr("网络错误"), false);
		else showTip(tr("未知错误"), false);
		return;
	}
	QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
	if (jsonDoc.isNull() != QJsonParseError::NoError) {
		showTip(tr("数据解析错误"), false);
		return;
	}
	if (!jsonDoc.isObject()) {
		showTip(tr("数据格式错误"), false);
		return;
	}
	_handlers[id](jsonDoc.object());
	return;
}

void ResetDialog::initHttpHandlers() {
	_handlers.insert(ReqId::ID_GET_verify_CODE, [this](const QJsonObject& obj) {
		int error = obj["error"].toInt();
		if (error != ErrorCodes::SUCCESS) {
			if (error == ErrorCodes::Err_Json) showTip(tr("json错误"), false);
			else showTip(tr("未知错误"), false);
			return;
		}
		auto email = obj["email"].toString();
		showTip(tr("验证码已发送!"), true);
		qDebug() << "verify code sent to " << email;
		});
	_handlers.insert(ReqId::ID_RESET_PWD, [this](QJsonObject jsonObj) {
		int error = jsonObj["error"].toInt();
		if (error != ErrorCodes::SUCCESS) {
			showTip(tr("注册失败:") + jsonObj["msg"].toString(), false);
			return;
		}
		auto email = jsonObj["email"].toString();
		showTip(tr("重置密码成功，请点击返回继续登录"), true);
		qDebug() << "user uuid is " << jsonObj["uid"].toString();
		qDebug() << "email is " << email;
		});
}
void ResetDialog::showTip(QString str, bool b_ok) {
	if (b_ok)
		ui->err_tip->setProperty("state", "ok");
	else
		ui->err_tip->setProperty("state", "error");
	qDebug() << "showTip: " << str;
	ui->err_tip->setText(str);
	repolish(ui->err_tip);
}

void ResetDialog::on_confirm_btn_clicked()
{
	if (!_tip_err.empty()) {
		showTip(_tip_err.begin().value(), false);
		return;
	}
	auto username = ui->user_edit->text();
	auto email = ui->email_edit->text();
	auto password = ui->pass_edit->text();
	auto verify = ui->verify_edit->text();
	if (username.isEmpty() || email.isEmpty() || password.isEmpty() || verify.isEmpty()) {
		showTip(tr("请填写完整信息"), false);
		return;
	}
	QJsonObject json_obj;
	json_obj["user"] = username;
	json_obj["email"] = email;
	json_obj["passwd"] = xorString(password);
	json_obj["verify_code"] = verify;
	HttpMgr::getInstance()->PostHttpReq(QUrl(gate_url_prefix + "/reset_pwd"), json_obj, ReqId::ID_RESET_PWD, Modules::RESETMOD);
}