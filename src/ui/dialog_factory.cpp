#include "ui/dialog_factory.hpp"
#include <obs-frontend-api.h>
#include <QDialog>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QApplication>
#include <QClipboard>
#include "core/qr_generator.hpp"
#include "mybili_api.hpp"

namespace UI {

static QDialog *createBaseDialog(const QString &title, QWidget *parent)
{
	QDialog *dialog = new QDialog(parent);
	dialog->setWindowTitle(title);
	QVBoxLayout *layout = new QVBoxLayout(dialog);
	dialog->setLayout(layout);
	return dialog;
}

void DialogFactory::message(const QString &msg, const QString &title, QWidget *parent)
{
	QDialog *dialog = createBaseDialog(title, parent ? parent : (QWidget *)obs_frontend_get_main_window());
	QVBoxLayout *layout = (QVBoxLayout *)dialog->layout();
	layout->addWidget(new QLabel(msg));
	QPushButton *confirm = new QPushButton("确认");
	layout->addWidget(confirm);
	QObject::connect(confirm, &QPushButton::clicked, dialog, &QDialog::accept);
	QObject::connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
	dialog->exec();
}

/**
 * QR login dialog:
 * 1. Call POST /api/user/qr/generate to get qrId + qrUrl
 * 2. Display qrUrl as a QR code in the dialog
 * 3. Poll POST /api/user/qr/status every 2 seconds
 * 4. If status == "confirmed", save config (token) and close dialog
 */
QDialog *DialogFactory::qrLogin(QWidget *parent, MyBili::Config &config)
{
	QDialog *dialog = createBaseDialog("mybilibili 扫码登录", parent);
	QVBoxLayout *layout = (QVBoxLayout *)dialog->layout();

	QLabel *qrLabel = new QLabel();
	QLabel *promptLabel = new QLabel("使用 mybilibili 手机客户端扫码登录");
	layout->addWidget(qrLabel);
	layout->addWidget(promptLabel);

	// Step 1: generate QR
	std::string qrId, qrUrl, genMsg;
	if (!MyBili::MbgApi::generateQr(qrId, qrUrl, genMsg)) {
		qrLabel->setText(QString::fromUtf8(genMsg));
		promptLabel->setText("生成二维码失败");
		QPushButton *closeBtn = new QPushButton("关闭");
		layout->addWidget(closeBtn);
		QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
		QObject::connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
		dialog->exec();
		return dialog;
	}

	QPixmap pixmap = Core::QrGenerator::generate(qrUrl);
	if (pixmap.isNull()) {
		qrLabel->setText("无法生成二维码");
		QPushButton *closeBtn = new QPushButton("关闭");
		layout->addWidget(closeBtn);
		QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
	} else {
		qrLabel->setPixmap(pixmap);
	}

	promptLabel->setText("请使用 mybilibili App 扫描二维码\n登录成功后请返回 OBS 继续操作");

	// Step 3: create timer to poll QR status
	QTimer *timer = new QTimer(dialog);
	int retryCount = 0;

	auto timerCallback = [=, &config]() mutable {
		if (retryCount > 150) { // 5 min
			timer->stop();
			promptLabel->setText("二维码已过期，请重新扫码");
			return;
		}
		++retryCount;

		std::string pollMsg;
		if (MyBili::MbgApi::pollQrStatus(qrId, config, pollMsg)) {
			timer->stop();
			promptLabel->setText("扫码成功！");
			QTimer::singleShot(500, dialog, [=]() {
				dialog->accept();
			});
		}
	};

	QObject::connect(timer, &QTimer::timeout, timerCallback);
	QObject::connect(dialog, &QDialog::finished, [timer, dialog]() {
		timer->stop();
		dialog->deleteLater();
	});

	timer->start(2000); // poll every 2 seconds
	dialog->exec();
	return dialog;
}

QDialog *DialogFactory::streamStarted(QWidget *parent, const std::string &rtmpAddr, const std::string &streamKey)
{
	QDialog *dialog = createBaseDialog("开始直播", parent);
	QVBoxLayout *layout = (QVBoxLayout *)dialog->layout();

	layout->addWidget(new QLabel(
		QString("推流信息已就绪，请在 OBS 设置中填入以下信息：\n\n"
			"服务: 自定义\n"
			"服务器: %1\n"
			"流密钥: %2\n\n"
			"然后点击 OBS 右下角的「开始直播」按钮。")
			.arg(QString::fromStdString(rtmpAddr), QString::fromStdString(streamKey))));

	QPushButton *copyFull = new QPushButton("复制服务器地址");
	QPushButton *copyKey = new QPushButton("复制流密钥");
	QPushButton *confirm = new QPushButton("已开始推流");

	QHBoxLayout *btnRow = new QHBoxLayout();
	btnRow->addWidget(copyFull);
	btnRow->addWidget(copyKey);
	layout->addLayout(btnRow);
	layout->addWidget(confirm);

	QObject::connect(copyFull, &QPushButton::clicked, [=]() {
		QApplication::clipboard()->setText(QString::fromStdString(rtmpAddr));
	});
	QObject::connect(copyKey, &QPushButton::clicked, [=]() {
		QApplication::clipboard()->setText(QString::fromStdString(streamKey));
	});
	QObject::connect(confirm, &QPushButton::clicked, dialog, &QDialog::accept);
	QObject::connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);

	dialog->exec();
	return dialog;
}

QDialog *DialogFactory::roomSettings(QWidget *parent, const std::string &currentRoomName,
				     std::function<void(const std::string &newName)> onApply)
{
	QDialog *dialog = createBaseDialog("更新房间名称", parent);
	QVBoxLayout *layout = (QVBoxLayout *)dialog->layout();

	QLineEdit *nameInput = new QLineEdit(QString::fromStdString(currentRoomName));
	layout->addWidget(new QLabel("直播间名称:"));
	layout->addWidget(nameInput);

	QPushButton *confirmBtn = new QPushButton("确认更新");
	layout->addWidget(confirmBtn);

	QObject::connect(confirmBtn, &QPushButton::clicked, [=]() {
		std::string newName = nameInput->text().trimmed().toUtf8().constData();
		if (!newName.empty() && onApply) {
			onApply(newName);
		}
		dialog->accept();
	});

	QObject::connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
	dialog->exec();
	return dialog;
}

} // namespace UI
