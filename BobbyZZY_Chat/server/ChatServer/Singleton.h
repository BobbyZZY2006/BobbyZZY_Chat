#pragma once
#include<iostream>
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
		std::cout << getInstance().get() << std::endl;
	}
	~Singleton() {
		std::cout << "Singleton destroyed" << std::endl;
	}
};