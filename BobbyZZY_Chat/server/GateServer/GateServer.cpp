#include"CServer.h""
#include<iostream>
#include "RedisMgr.h"
#include "const.h"
#include <memory>

int main() {
        auto &g_config_mgr = *(ConfigMgr::getInstance());
        std::string gate_port_str = g_config_mgr["GateServer"]["port"];
        unsigned short port = atoi(gate_port_str.c_str());
            net::io_context ioc{ 1 };
            boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
            signals.async_wait([&ioc](const beast::error_code&error, int signal_number) {
                if (error) {
                    return;
                }
                ioc.stop();
            });
            std::make_shared<CServer>(ioc, port)->Start();
            std::cout << "server start at port " << port << std::endl;
            ioc.run();
}