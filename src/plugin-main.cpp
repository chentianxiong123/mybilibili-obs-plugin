#include <obs-data.h>
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QMainWindow>
#include <QDesktopServices>
#include <QUrl>
#include "ui/menu_manager.hpp"
#include "ui/dialog_factory.hpp"
#include "core/config_manager.hpp"
#include "core/qr_generator.hpp"
#include "mybili_api.hpp"
#include "plugin_utils.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

class MybilibiliPlugin : public QObject {
	Q_OBJECT
public:
	explicit MybilibiliPlugin(QMainWindow *parent);
	~MybilibiliPlugin();

private slots:
	void onScanQrcode();
	void onStreamToggle();
	void onOpenRoom();
	void onUpdateRoomInfo();

private:
	void updateLoginStatus();
	void openLiveRoom();
	void fetchRoomInfo();

	Core::ConfigManager m_config;
	UI::MenuManager *m_menu;
};

MybilibiliPlugin::MybilibiliPlugin(QMainWindow *parent)
	: QObject(nullptr), m_menu(new UI::MenuManager(parent->menuBar(), this))
{
	m_config.load();

	connect(m_menu, &UI::MenuManager::scanQrcodeClicked, this, &MybilibiliPlugin::onScanQrcode);
	connect(m_menu, &UI::MenuManager::streamToggleClicked, this, &MybilibiliPlugin::onStreamToggle);
	connect(m_menu, &UI::MenuManager::openRoomClicked, this, &MybilibiliPlugin::onOpenRoom);
	connect(m_menu, &UI::MenuManager::updateRoomInfoClicked, this, &MybilibiliPlugin::onUpdateRoomInfo);

	// Check if we have a token and try to restore login
	auto &cfg = m_config.config();
	if (!cfg.token.empty()) {
		// Try to fetch room info to verify token validity
		std::string msg;
		if (MyBili::MbgApi::getMyRoom(cfg, msg)) {
			cfg.loginStatus = true;
			m_config.save();
		}
	}

	updateLoginStatus();
	if (cfg.streaming) {
		m_menu->actions().streamToggle->setText("停止直播");
	}
}

MybilibiliPlugin::~MybilibiliPlugin()
{
	obs_log(LOG_DEBUG, "释放 mybilibili 插件资源");
}

void MybilibiliPlugin::updateLoginStatus()
{
	auto &cfg = m_config.config();
	m_menu->actions().loginStatus->setText(cfg.loginStatus ? "登录状态: 已登录" : "登录状态: 未登录");
	m_menu->actions().loginStatus->setChecked(cfg.loginStatus);
}

void MybilibiliPlugin::openLiveRoom()
{
	auto &cfg = m_config.config();
	if (cfg.roomId <= 0) {
		UI::DialogFactory::message(QString::fromUtf8("没有直播间信息，请先登录"), "消息");
		return;
	}
	const QString url = QString("https://m.mybilibili.cn/m/live/%1").arg(cfg.roomId);
	if (!QDesktopServices::openUrl(QUrl(url))) {
		UI::DialogFactory::message(QStringLiteral("无法打开浏览器，请手动访问：\n") + url, "消息");
	}
}

void MybilibiliPlugin::onOpenRoom()
{
	openLiveRoom();
}

void MybilibiliPlugin::onScanQrcode()
{
	auto &cfg = m_config.config();
	auto parent = (QWidget *)obs_frontend_get_main_window();

	UI::DialogFactory::qrLogin(parent, cfg);

	// After QR dialog closes, check if we have a token
	if (!cfg.token.empty()) {
		std::string msg;
		cfg.loginStatus = MyBili::MbgApi::getMyRoom(cfg, msg);
		if (cfg.loginStatus) {
			m_config.save();
			updateLoginStatus();
			UI::DialogFactory::message(
				QString::fromUtf8(("登录成功！直播间: " + cfg.roomName).c_str()),
				"消息");
		} else {
			cfg.loginStatus = false;
			UI::DialogFactory::message(QString::fromUtf8(msg), "登录失败");
		}
	}
}

void MybilibiliPlugin::onStreamToggle()
{
	auto &cfg = m_config.config();
	std::string message;

	if (cfg.streaming) {
		if (MyBili::MbgApi::stopLive(cfg, message)) {
			m_menu->actions().streamToggle->setText("开始直播");
			cfg.streaming = false;
			m_config.save();
			UI::DialogFactory::message(QString::fromUtf8("直播已停止"), "消息");
		} else {
			UI::DialogFactory::message(QString::fromUtf8(message), "消息");
		}
	} else {
		if (!cfg.loginStatus || cfg.roomId <= 0) {
			UI::DialogFactory::message(QString::fromUtf8("请先扫码登录"), "消息");
			return;
		}

		// Mark as live in backend
		if (MyBili::MbgApi::startLive(cfg, message)) {
			m_menu->actions().streamToggle->setText("停止直播");
			cfg.streaming = true;
			m_config.save();
			// Show RTMP info dialog
			UI::DialogFactory::streamStarted(
				(QWidget *)obs_frontend_get_main_window(),
				cfg.rtmpAddr,
				cfg.streamKey);
		} else {
			UI::DialogFactory::message(QString::fromUtf8(message), "开播失败");
		}
	}
}

void MybilibiliPlugin::onUpdateRoomInfo()
{
	auto &cfg = m_config.config();
	auto parent = (QWidget *)obs_frontend_get_main_window();

	UI::DialogFactory::roomSettings(parent, cfg.roomName,
		[this, &cfg](const std::string &newName) {
			std::string message;
			if (MyBili::MbgApi::updateRoomInfo(cfg, newName, message)) {
				m_config.save();
				UI::DialogFactory::message(
					QString::fromUtf8(("房间名称已更新为: " + newName).c_str()),
					"消息");
			} else {
				UI::DialogFactory::message(
					QString::fromUtf8(message),
					"更新失败");
			}
		});
}

static MybilibiliPlugin *plugin = nullptr;

bool obs_module_load(void)
{
	obs_log(LOG_INFO, "mybilibili 插件版本: %s, commit: %s", PLUGIN_VERSION, GIT_COMMIT_HASH);
	MyBili::MbgApi::init();
	auto mainWindow = static_cast<QMainWindow *>(obs_frontend_get_main_window());
	if (!mainWindow)
		return false;
	plugin = new MybilibiliPlugin(mainWindow);
	obs_log(LOG_INFO, "mybilibili 插件加载成功");
	return true;
}

void obs_module_unload(void)
{
	MyBili::MbgApi::cleanup();
	delete plugin;
	plugin = nullptr;
	obs_log(LOG_INFO, "mybilibili 插件已卸载");
}

#include "plugin-main.moc"