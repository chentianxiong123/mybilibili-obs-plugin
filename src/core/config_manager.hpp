#pragma once
#include "mybili_api.hpp"

namespace Core {
class ConfigManager {
public:
	ConfigManager();
	void load();
	void save();
	MyBili::Config &config() { return m_config; }
	const MyBili::Config &config() const { return m_config; }

private:
	MyBili::Config m_config;
	char *getConfigPath();
};
} // namespace Core
