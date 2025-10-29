#include "ConfigMgr.h"

ConfigMgr::ConfigMgr() {
	boost::filesystem::path current_path = boost::filesystem::current_path();
	boost::filesystem::path config_path = current_path / "config.ini";
	std::cout << "config path is " << config_path.string() << std::endl;
	boost::property_tree::ptree pt;
 	boost::property_tree::read_ini(config_path.string(), pt);

	for(const auto & section : pt) {
		std::map<std::string, std::string> key_map;
		for (const auto& key : section.second) {
			key_map.insert(std::make_pair(key.first, key.second.get_value<std::string>()));
		}
		_config_map.insert(std::make_pair(section.first, key_map));
	}
	for(const auto & section : _config_map) {
		std::cout << "[" << section.first << "]"<<std::endl;
		for (const auto& key : section.second) {
			std::cout << "key:" << key.first << " value:" << key.second << std::endl;
		}
	}
}