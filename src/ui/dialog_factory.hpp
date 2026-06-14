#pragma once
#include <QWidget>
#include <QString>
#include <string>
#include <functional>
#include "mybili_api.hpp"

namespace UI {
class DialogFactory {
public:
	static void message(const QString &msg, const QString &title, QWidget *parent = nullptr);
	static QDialog *qrLogin(QWidget *parent, MyBili::Config &config);
	static QDialog *streamStarted(QWidget *parent, const std::string &rtmpAddr, const std::string &streamKey);
	static QDialog *roomSettings(QWidget *parent, const std::string &currentRoomName,
				     std::function<void(const std::string &newName)> onApply);
};
} // namespace UI
