#include "ui/menu_manager.hpp"
#include <QMenu>

namespace UI {
MenuManager::MenuManager(QMenuBar *menuBar, QObject *parent)
	: QObject(parent), m_menuBar(menuBar)
{
	setupMenu();
}

void MenuManager::setupMenu()
{
	QMenu *biliMenu = m_menuBar->addMenu("mybilibili");
	QMenu *loginMenu = biliMenu->addMenu("登录");

	m_actions.scanQrcode = loginMenu->addAction("扫码登录");
	m_actions.loginStatus = loginMenu->addAction("登录状态: 未登录");
	m_actions.loginStatus->setCheckable(true);
	m_actions.loginStatus->setEnabled(false);

	m_actions.streamToggle = biliMenu->addAction("开始直播");
	m_actions.openRoom = biliMenu->addAction("打开直播间");
	m_actions.updateRoomInfo = biliMenu->addAction("更新房间名称");

	connect(m_actions.scanQrcode, &QAction::triggered, this, &MenuManager::scanQrcodeClicked);
	connect(m_actions.streamToggle, &QAction::triggered, this, &MenuManager::streamToggleClicked);
	connect(m_actions.openRoom, &QAction::triggered, this, &MenuManager::openRoomClicked);
	connect(m_actions.updateRoomInfo, &QAction::triggered, this, &MenuManager::updateRoomInfoClicked);
}
} // namespace UI
