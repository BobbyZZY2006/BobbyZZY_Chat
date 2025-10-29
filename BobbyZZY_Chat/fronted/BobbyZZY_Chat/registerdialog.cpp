#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "HttpMgr.h"

RegisterDialog::RegisterDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::RegisterDialog)
{

    ui->setupUi(this);
	ui->pass_edit->setEchoMode(QLineEdit::Password);
	ui->confirm_edit->setEchoMode(QLineEdit::Password);
	ui->err_tip->clear();
	ui->err_tip->setProperty("state", "ok");
	repolish(ui->err_tip);
	QToolButton* passwd_visible_btn = new QToolButton(ui->pass_edit);
	QToolButton* confirm_visible_btn = new QToolButton(ui->confirm_edit);
	passwd_visible_btn->setObjectName("passwd_visible_btn");
	confirm_visible_btn->setObjectName("confirm_visible_btn");
	passwd_visible_btn->setCursor(Qt::PointingHandCursor);
	passwd_visible_btn->setCheckable(true);
	confirm_visible_btn->setCursor(Qt::PointingHandCursor);
	confirm_visible_btn->setCheckable(true);
	ui->pass_edit->setTextMargins(0, 0, passwd_visible_btn->sizeHint().width(), 0);
	ui->confirm_edit->setTextMargins(0, 0, confirm_visible_btn->sizeHint().width(), 0);
	connect(passwd_visible_btn, &QToolButton::toggled, this, [this](bool checked) {
		if(checked)  ui->pass_edit->setEchoMode(QLineEdit::Normal);
		else  ui->pass_edit->setEchoMode(QLineEdit::Password);
		qDebug() << "passwd_visible_btn checked =" << checked;
		});
	connect(confirm_visible_btn, &QToolButton::toggled, this, [this](bool checked) {
		if (checked)  ui->confirm_edit->setEchoMode(QLineEdit::Normal);
		else  ui->confirm_edit->setEchoMode(QLineEdit::Password);
		qDebug() << "confirm_visible_btn checked =" << checked;
		});

	// 用布局把按钮固定在右边
	auto layout = new QHBoxLayout(ui->pass_edit);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addStretch();
	layout->addWidget(passwd_visible_btn);
	layout = new QHBoxLayout(ui->confirm_edit);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addStretch();
	layout->addWidget(confirm_visible_btn);

	connect(ui->user_edit, &QLineEdit::editingFinished, this, [this]() {
		checkUserValid();
		});

	connect(ui->email_edit, &QLineEdit::editingFinished, this, [this]() {
		checkEmailValid();
		});

	connect(ui->pass_edit, &QLineEdit::editingFinished, this, [this]() {
		checkPwdValid();
		});

	connect(ui->confirm_edit, &QLineEdit::editingFinished, this, [this]() {
		checkConfirmValid();
		});

	connect(ui->verify_edit, &QLineEdit::editingFinished, this, [this]() {
		checkVerifyValid();
		});
	connect(HttpMgr::getInstance().get(), &HttpMgr::sig_reg_mod_finish, 
		this, &RegisterDialog::slot_reg_mod_finish);
	initHttpHandlers();

	// 创建定时器
	_countdown_timer = new QTimer(this);
	// 连接信号和槽
	connect(_countdown_timer, &QTimer::timeout, [this]() {
		if (_countdown == 0) {
			_countdown_timer->stop();
			emit sigSwitchLogin();
			return;
		}
		_countdown--;
		auto str = QString("注册成功，%1 s后返回登录").arg(_countdown);
		ui->tip_lb->setText(str);
		});
}

void RegisterDialog::on_return_btn_clicked()
{
	_countdown_timer->stop();
	emit sigSwitchLogin();
}

void RegisterDialog::on_cancel_btn_clicked()
{
	_countdown_timer->stop();
	emit sigSwitchLogin();
}

void RegisterDialog::ChangeTipPage()
{
	_countdown_timer->stop();
	ui->stackedWidget->setCurrentWidget(ui->page_2);

	// 启动定时器，设置间隔为1000毫秒（1秒）
	_countdown_timer->start(5000);
}

void RegisterDialog::addTipErr(TipErr err, QString tip) {
	_tip_err[err] = tip;
	showTip(tip, false);
}

void RegisterDialog::removeTipErr(TipErr err) {
	_tip_err.remove(err);
	if (_tip_err.empty()) {
		ui->err_tip->clear();
	}
	else {
		showTip(_tip_err.begin().value(), false);
	}
}

void RegisterDialog::checkPwdValid() {
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

void RegisterDialog::checkUserValid() {
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

void RegisterDialog::checkEmailValid() {
	auto email = ui->email_edit->text();
	QRegularExpression email_regex(R"([A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,})");
	bool match = email_regex.match(email).hasMatch();
	if (!match) {
		addTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱格式错误"));
		return;
	}
	removeTipErr(TipErr::TIP_EMAIL_ERR);
}

void RegisterDialog::checkConfirmValid() {
	auto pwd = ui->pass_edit->text();
	auto confirm = ui->confirm_edit->text();
	if (pwd != confirm) {
		addTipErr(TipErr::TIP_CONFIRM_ERR, tr("两次密码输入不一致"));
		return;
	}
	removeTipErr(TipErr::TIP_CONFIRM_ERR);
}

void RegisterDialog::checkVerifyValid() {
	auto verify = ui->verify_edit->text();
	if (verify.length() != 4) {
		addTipErr(TipErr::TIP_VERIFY_ERR, tr("验证码长度应为4"));
		return;
	}
	removeTipErr(TipErr::TIP_VERIFY_ERR);
}

RegisterDialog::~RegisterDialog()
{
    delete ui;
}

void RegisterDialog::on_get_code_clicked() {
	if (!ui->email_edit->text().length()) showTip(tr("邮箱不能为空！"), false);
    else if (_tip_err.empty()) {
		ui->get_code->changeEvent();
		QJsonObject json_obj;
		json_obj["email"] = ui->email_edit->text();
		HttpMgr::getInstance()->PostHttpReq(QUrl(gate_url_prefix+"/get_verifycode"),json_obj,ReqId::ID_GET_verify_CODE,Modules::REGISTERMOD);
    }

}

void RegisterDialog::slot_reg_mod_finish(ReqId id, QString res, ErrorCodes err) {
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

void RegisterDialog::initHttpHandlers() {
	_handlers.insert(ReqId::ID_GET_verify_CODE, [this](const QJsonObject& obj) {
		int error = obj["error"].toInt();
		if (error != ErrorCodes::SUCCESS) {
			if(error== ErrorCodes::Err_Json) showTip(tr("json错误"), false);
			else if (!obj["msg"].isNull()) showTip(tr("注册失败:") + obj["msg"].toString(), false);
			else showTip(tr("未知错误"), false);
			return;
		}
		auto email = obj["email"].toString();
		showTip(tr("验证码已发送!"), true);
		qDebug() << "verify code sent to " << email;
		});
	_handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj) {
		int error = jsonObj["error"].toInt();
		if (error != ErrorCodes::SUCCESS) {
			if(!jsonObj["msg"].isNull()) showTip(tr("注册失败:") + jsonObj["msg"].toString(), false);
			else showTip(tr("未知错误"), false); 
			return;
		}
		auto email = jsonObj["email"].toString();
		showTip(tr("用户注册成功"), true);
		qDebug() << "user uuid is " << jsonObj["uid"].toString();
		qDebug() << "email is " << email;
		ChangeTipPage();
		});
}
void RegisterDialog::showTip(QString str,bool b_ok) {
    if (b_ok)
		ui->err_tip->setProperty("state", "ok");
	else
		ui->err_tip->setProperty("state", "error");
	qDebug() << "showTip: " << str;
	ui->err_tip->setText(str);
	repolish(ui->err_tip);
}

void RegisterDialog::on_confirm_btn_clicked()
{
	if(!_tip_err.empty()) {
		showTip(_tip_err.begin().value(), false);
		return;
	}
	auto username = ui->user_edit->text();
	auto email = ui->email_edit->text();
	auto password = ui->pass_edit->text();
	auto confirm = ui->confirm_edit->text();
	auto verify = ui->verify_edit->text();
	if (username.isEmpty() || email.isEmpty() || password.isEmpty() || confirm.isEmpty() || verify.isEmpty()) {
		showTip(tr("请填写完整信息"), false);
		return;
	}
	if (password != confirm) {
		showTip(tr("两次密码输入不一致"), false);
		return;
	}
	QJsonObject json_obj;
	json_obj["username"] = username;
	json_obj["email"] = email;
	json_obj["password"] = xorString(password);
	json_obj["verify_code"] = verify;
	HttpMgr::getInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_register"), json_obj, ReqId::ID_REG_USER, Modules::REGISTERMOD);
}