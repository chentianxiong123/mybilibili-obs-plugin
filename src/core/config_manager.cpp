#include "core/config_manager.hpp"
#include <obs-module.h>

namespace Core {
ConfigManager::ConfigManager() {}

char *ConfigManager::getConfigPath()
{
	return obs_module_file("config.json");
}

void ConfigManager::load()
{
	m_config = MyBili::Config();

	char *configFile = getConfigPath();
	if (!configFile)
		return;

	obs_data_t *settings = obs_data_create_from_json_file(configFile);
	if (!settings) {
		bfree(configFile);
		return;
	}

	m_config.token = obs_data_get_string(settings, "token");
	m_config.refreshToken = obs_data_get_string(settings, "refreshToken");
	m_config.roomId = static_cast<int>(obs_data_get_int(settings, "roomId"));
	m_config.userId = static_cast<int>(obs_data_get_int(settings, "userId"));
	m_config.roomName = obs_data_get_string(settings, "roomName");
	m_config.streamKey = obs_data_get_string(settings, "streamKey");
	m_config.rtmpAddr = obs_data_get_string(settings, "rtmpAddr");
	obs_data_release(settings);
	bfree(configFile);

	if (m_config.roomName.empty())
		m_config.roomName = "我的直播间";
	if (m_config.rtmpAddr.empty())
		m_config.rtmpAddr = "rtmp://localhost/live";
}

void ConfigManager::save()
{
	char *configFile = getConfigPath();
	if (!configFile)
		return;

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "token", m_config.token.c_str());
	obs_data_set_string(settings, "refreshToken", m_config.refreshToken.c_str());
	obs_data_set_int(settings, "roomId", m_config.roomId);
	obs_data_set_int(settings, "userId", m_config.userId);
	obs_data_set_string(settings, "roomName", m_config.roomName.c_str());
	obs_data_set_string(settings, "streamKey", m_config.streamKey.c_str());
	obs_data_set_string(settings, "rtmpAddr", m_config.rtmpAddr.c_str());
	obs_data_save_json(settings, configFile);
	obs_data_release(settings);
	bfree(configFile);
}
} // namespace Core
