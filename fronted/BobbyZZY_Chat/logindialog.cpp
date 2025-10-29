#include "logindialog.h"
#include "ui_logindialog.h"
#include "HttpMgr.h"
#include "TcpMgr.h"

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    ui->forget_button->setCursor(Qt::PointingHandCursor);
    ui->err_tip->clear();
    ui->pass_edit->setEchoMode(QLineEdit::Password);
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


    connect(ui->reg_btn,&QPushButton::clicked,this,&LoginDialog::switchRegister);
    connect(ui->forget_button, &QToolButton::clicked, this, &LoginDialog::slot_forget_pwd);
    ui->loginpng->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    initUI();
	initHttpHandlers();

    connect(HttpMgr::getInstance().get(), &HttpMgr::sig_login_mod_finish, this,
		&LoginDialog::slot_login_mod_finish);
    connect(this, &LoginDialog::sig_connect_tcp, TcpMgr::getInstance().get(), &TcpMgr::slot_tcp_connect);
	connect(TcpMgr::getInstance().get(), &TcpMgr::sig_tcp_con_res, this, &LoginDialog::slot_tcp_con_res);
	connect(TcpMgr::getInstance().get(), &TcpMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);
}

void LoginDialog::initUI()
{
	QPixmap pixmap(":/res/login.png");
    qDebug() << "pixmap size:" << pixmap.size() <<' '<<ui->loginpng->size();
    QPixmap scaled = pixmap.scaled(ui->loginpng->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    qDebug() << "scaled size:" << scaled.size();
	ui->loginpng->setPixmap(scaled);
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::slot_forget_pwd()
{
    qDebug() << "slot forget pwd";
    emit switchReset();
}

void LoginDialog::on_login_btn_clicked()
{
    qDebug() << "login btn clicked";
	ui->err_tip->clear();
    if (checkUserValid() == false) {
        return;
    }

    if (checkPwdValid() == false) {
        return;
    }

    auto user_email = ui->user_edit->text();
    auto pwd = ui->pass_edit->text();
    //发送http请求登录
    QJsonObject json_obj;
    json_obj["email"] = user_email;
    json_obj["passwd"] = xorString(pwd);
    HttpMgr::getInstance()->PostHttpReq(QUrl(gate_url_prefix + "/user_login"),
        json_obj, ReqId::ID_LOGIN_USER, Modules::LOGINMOD);
}

bool LoginDialog::checkUserValid() {
        auto email = ui->user_edit->text();
        QRegularExpression email_regex(R"([A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,})");
        bool match = email_regex.match(email).hasMatch();
        if (!match) {
            showTip(tr("邮箱格式错误"),false);
            return false;
        }
        return true;
}

bool LoginDialog::checkPwdValid() {
    auto pwd = ui->pass_edit->text();
    if (pwd.length() < 6 || pwd.length() > 15) {
		showTip(tr("密码长度应为6~15"), false);
        qDebug() << "Pass length invalid";
        return false;
    }

    return true;
}

void LoginDialog::showTip(QString str, bool b_ok) {
    if (b_ok)
        ui->err_tip->setProperty("state", "ok");
    else
        ui->err_tip->setProperty("state", "error");
    qDebug() << "showTip: " << str;
    ui->err_tip->setText(str);
    repolish(ui->err_tip);
}

void LoginDialog::initHttpHandlers() {
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](const QJsonObject& obj) {
        int error = obj["error"].toInt();
        if (error != ErrorCodes::SUCCESS) {
            if (!obj["msg"].isNull()) showTip(tr("注册失败:") + obj["msg"].toString(), false);
            else showTip(tr("未知错误"), false);
            return;
        }
        auto email = obj["email"].toString();
        showTip(tr("用户登录成功!"), true);


		_si.uid = obj["uid"].toInt();
        _si.host = obj["host"].toString();
        _si.port = obj["port"].toString();
        _si.token = obj["token"].toString();

        emit sig_connect_tcp(_si);

        });
}
void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        showTip(tr("网络请求错误"), false);
        return;
    }

    // 解析 JSON 字符串,res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //json解析错误
    if (jsonDoc.isNull()) {
        showTip(tr("json解析错误"), false);
        return;
    }

    //json解析错误
    if (!jsonDoc.isObject()) {
        showTip(tr("json解析错误"), false);
        return;
    }

    //调用对应的逻辑,根据id回调。
    _handlers[id](jsonDoc.object());

    return;
}

void LoginDialog::slot_tcp_con_res(bool bsuccess)
{
    if (bsuccess) {
        showTip(tr("聊天服务连接成功，正在登录..."), true);
        QJsonObject jsonObj;
        jsonObj["uid"] = _si.uid;
        jsonObj["token"] = _si.token;
        QJsonDocument doc(jsonObj);
        QString jsonString = doc.toJson(QJsonDocument::Indented);
        //发送tcp请求给chat server
        emit TcpMgr::getInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonString);
    }
    else {
        showTip(tr("网络异常"), false);
    }
}

void LoginDialog::slot_login_failed(int err)
{
	QString result = QString("登录失败，err is %1").arg(err);
	showTip(result, false);
  
}
