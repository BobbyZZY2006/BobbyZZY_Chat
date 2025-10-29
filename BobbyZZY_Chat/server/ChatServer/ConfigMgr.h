#pragma once
#include "const.h"

class ConfigMgr :public Singleton<ConfigMgr>
{
public:
	friend class Singleton<ConfigMgr>;
	~ConfigMgr() {
		_config_map.clear();
	}
	std::map<std::string,std::string> operator[](std::string section) {
		auto it = _config_map.find(section);
		if(it != _config_map.end()) {
			return it->second;
		}
		return std::map<std::string, std::string>();
	}

	//ConfigMgr(const ConfigMgr& src) {
	//	_config_map = src._config_map;
	//}
	//ConfigMgr& operator=(const ConfigMgr& src) {
	//	if(this != &src) {
	//		_config_map = src._config_map;
	//	}
	//	return *this;
	//}
	
private:
	ConfigMgr();
	std::map<std::string, std::map<std::string,std::string>> _config_map;
};

