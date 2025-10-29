#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <iostream>
#include <chrono>
#include <thread>

SqlConnection::SqlConnection(mysqlx::Session* sess, int64_t lasttime)
    : _sess(sess), _last_oper_time(lasttime) {}

MySqlPool::MySqlPool(const std::string& host, int port,
                     const std::string& user, const std::string& pass,
                     const std::string& schema, int poolSize)
    : _host(host), _port(port), _user(user), _pass(pass),
      _schema(schema), _poolSize(poolSize), _b_stop(false) {
    try {
        for (int i = 0; i < _poolSize; ++i) {
            auto* sess = new mysqlx::Session(_host, _port, _user, _pass);
            sess->sql("USE " + _schema).execute();
            auto now = std::chrono::system_clock::now().time_since_epoch();
            int64_t ts = std::chrono::duration_cast<std::chrono::seconds>(now).count();
            _pool.push(std::make_unique<SqlConnection>(sess, ts));
        }
        _check_thread = std::thread([this]() {
            while (!_b_stop) {
                checkConnection();
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }
        });
    } catch (const mysqlx::Error& e) {
        std::cerr << "MySQL pool init failed: " << e.what() << std::endl;
    }
}

void MySqlPool::checkConnection() {
    std::lock_guard<std::mutex> lock(_mutex);
    auto pool_size = _pool.size();
    auto now = std::chrono::system_clock::now().time_since_epoch();
    int64_t ts = std::chrono::duration_cast<std::chrono::seconds>(now).count();
    for (size_t i = 0; i < pool_size; i++) {
        auto con = std::move(_pool.front());
        _pool.pop();
        if (ts - con->_last_oper_time < 300) {
            _pool.push(std::move(con));
            continue;
        }
        try {
            con->_sess->sql("SELECT 1").execute();
            con->_last_oper_time = ts;
        } catch (const mysqlx::Error&) {
            auto* new_sess = new mysqlx::Session(_host, _port, _user, _pass);
            new_sess->sql("USE " + _schema).execute();
            con->_sess.reset(new_sess);
            con->_last_oper_time = ts;
        }
        _pool.push(std::move(con));
    }
}

void MySqlPool::returnConnection(std::unique_ptr<SqlConnection> con) {
    if (!con) return;
    std::unique_lock<std::mutex> lock(_mutex);
    if (_b_stop) return;
    _pool.push(std::move(con));
    _cv.notify_one();
}

std::unique_ptr<SqlConnection> MySqlPool::getConnection() {
    std::unique_lock<std::mutex> lock(_mutex);
    _cv.wait(lock, [this] { return _b_stop || !_pool.empty(); });
    if (_b_stop) return nullptr;
    auto con = std::move(_pool.front());
    _pool.pop();
    return con;
}

void MySqlPool::Close() {
    _b_stop = true;
    _cv.notify_all();
}

MySqlPool::~MySqlPool() {
    Close();
    if (_check_thread.joinable()) _check_thread.join();
    std::unique_lock<std::mutex> lock(_mutex);
    while (!_pool.empty()) _pool.pop();
}

MysqlDao::MysqlDao() {
    auto& g_config_mgr = *(ConfigMgr::getInstance());
    std::string host = g_config_mgr["Mysql"]["host"];
    int port = std::stoi(g_config_mgr["Mysql"]["port"]);
    std::string user = g_config_mgr["Mysql"]["user"];
    std::string schema = g_config_mgr["Mysql"]["schema"];
    std::string password = g_config_mgr["Mysql"]["passwd"];
    _pool = std::make_unique<MySqlPool>(host, port, user, password, schema, 10);
}

MysqlDao::~MysqlDao() {
    _pool->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& password, const std::string& email) {
    auto con = _pool->getConnection();
    if (!con) return 0;
    try {
        con->_sess->sql("CALL reg_user(?,?,?,@result)")
            .bind(name, email, password).execute();
        auto rs = con->_sess->sql("SELECT @result AS result").execute();
        if (rs.count() > 0) {
            int result = rs.fetchOne()[0].get<int>();
            _pool->returnConnection(std::move(con));
            return result;
        }
    } catch (const mysqlx::Error& e) {
        std::cerr << "MySQL exception: " << e.what() << std::endl;
    }
    _pool->returnConnection(std::move(con));
    return 0;
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
    auto con = _pool->getConnection();
    if (!con) return false;
    try {
        auto rs = con->_sess->sql("SELECT email FROM user WHERE name=?")
            .bind(name).execute();
        for (auto row : rs) {
            if (row[0].get<std::string>() == email) {
                _pool->returnConnection(std::move(con));
                return true;
            }
        }
    } catch (const mysqlx::Error& e) {
        std::cerr << "MySQL exception: " << e.what() << std::endl;
    }
    _pool->returnConnection(std::move(con));
    return false;
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
    auto con = _pool->getConnection();
    if (!con) return false;
    try {
        auto res = con->_sess->sql("UPDATE user SET pwd=? WHERE name=?")
            .bind(newpwd, name).execute();
        _pool->returnConnection(std::move(con));
        return res.getAffectedItemsCount() > 0;
    } catch (const mysqlx::Error& e) {
        std::cerr << "MySQL exception: " << e.what() << std::endl;
    }
    _pool->returnConnection(std::move(con));
    return false;
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
    auto con = _pool->getConnection();
    if (!con) return false;
    try {
        auto rs = con->_sess->sql("SELECT uid, name, email, pwd FROM user WHERE email=?")
            .bind(email).execute();
        if (rs.count() == 0) {
            _pool->returnConnection(std::move(con));
            return false;
        }
        auto row = rs.fetchOne();
        std::string origin_pwd = row[3].get<std::string>();
        if (pwd != origin_pwd) {
            _pool->returnConnection(std::move(con));
            return false;
        }
        userInfo.user_id   = row[0].get<int64_t>();
        userInfo.user_name = row[1].get<std::string>();
        userInfo.email     = row[2].get<std::string>();
        userInfo.password  = origin_pwd;
        _pool->returnConnection(std::move(con));
        return true;
    } catch (const mysqlx::Error& e) {
        std::cerr << "MySQL exception: " << e.what() << std::endl;
    }
    _pool->returnConnection(std::move(con));
    return false;
}
