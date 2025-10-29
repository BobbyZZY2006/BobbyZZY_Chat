#pragma once
#include"global.h"
using namespace std;
template<typename T>
class Singleton
{
protected:
	Singleton() = default;
	Singleton(const Singleton<T>&) = delete;
	Singleton& operator=(const Singleton<T>&) = delete;
public:
	static std::shared_ptr<T> getInstance()
	{
		static std::shared_ptr<T> _instance(new T());
		return _instance;
	}
	
	void printAddress() {
		cout << getInstance().get() << endl;
	}
	~Singleton() {
		cout << "Singleton destroyed" << endl;
	}
};  