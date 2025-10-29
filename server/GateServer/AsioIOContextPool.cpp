#include "AsioIOContextPool.h"

AsioIOContextPool::AsioIOContextPool(std::size_t size) :_ioContexts(size),
_workGuards(size), _nextIOContext(0) {
	for (std::size_t i = 0; i < size; i++) {
		_workGuards[i] = std::make_unique<WorkGuard>
			(boost::asio::make_work_guard(_ioContexts[i].get_executor()));
	}
	//遍历多个iocontext，创建多个线程，每个线程内部启动iocontext

	for (std::size_t i = 0; i < _ioContexts.size(); i++) {
		_threads.emplace_back([this, i]() {
			_ioContexts[i].run();
			});
	}
}

AsioIOContextPool::~AsioIOContextPool() {
	Stop(); 
	std::cout << "AsIOContextPool destruct" << std::endl;
}

boost::asio::io_context& AsioIOContextPool::GetIOContext() {
	auto& service = _ioContexts[_nextIOContext++];
	if (_nextIOContext == _ioContexts.size()) {
		_nextIOContext = 0;
	}
	return service;
}

void AsioIOContextPool::Stop() {
	for (auto& work : _workGuards) {
		work.reset();
	}

	for (auto& t : _threads) {
		t.join();
	}
}

