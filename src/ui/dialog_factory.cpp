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
#include "bilibili_api.hpp"

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

QDialog *DialogFactory::qrLogin(QWidget *parent, const std::string &qrData, std::string &qrKey,
                std::function<void(const std::string &cookies)> onSuccess)
{
    QDialog *dialog = createBaseDialog("mybilibili 登录二维码", parent);
    QVBoxLayout *layout = (QVBoxLayout *)dialog->layout();

    QLabel *qrLabel = new QLabel();
    QPixmap pixmap = Core::QrGenerator::generate(qrData);
    if (pixmap.isNull()) {
        qrLabel->setText("无法生成二维码");
    } else {
        qrLabel->setPixmap(pixmap);
    }
    layout->addWidget(qrLabel);
    layout->addWidget(new QLabel("使用手机扫描二维码登录"));

    QTimer *timer = new QTimer(dialog);
    int retryCount = 0;

    auto timerCallback = [&, retryCount, qrKey, onSuccess, timer, qrLabel, dialog]() mutable {
        if (retryCount > 180) {
            timer->stop();
            qrLabel->setText("二维码已过期，请重新打开");
            return;
        }
        ++retryCount;

        std::string cookies, message;
        if (Bili::BiliApi::qrLogin(qrKey, cookies, message)) {
            timer->stop();
            if (onSuccess)
                onSuccess(cookies);
            dialog->accept();
        }
    };

    QObject::connect(timer, &QTimer::timeout, timerCallback);
    QObject::connect(dialog, &QDialog::finished, [timer, dialog]() {
        timer->stop();
        dialog->deleteLater();
    });

    timer->start(1000);
    dialog->exec();
    return dialog;
}

QDialog *DialogFactory::streamStarted(QWidget *parent, const std::string &rtmpAddr, const std::string &rtmpCode)
{
    QDialog *dialog = createBaseDialog("消息", parent);
    QVBoxLayout *layout = (QVBoxLayout *)dialog->layout();

    layout->addWidget(new QLabel(
        QString("直播已开始，请复制以下内容进行推流\n"
            "RTMP 地址: %1\n推流码: %2")
            .arg(QString::fromStdString(rtmpAddr), QString::fromStdString(rtmpCode))));

    QPushButton *copy = new QPushButton("复制");
    QPushButton *confirm = new QPushButton("确认");
    layout->addWidget(copy);
    layout->addWidget(confirm);

    QObject::connect(copy, &QPushButton::clicked, [=]() {
        QApplication::clipboard()->setText(QString("推流地址: %1\n推流码: %2")
                          .arg(QString::fromStdString(rtmpAddr), QString::fromStdString(rtmpCode)));
    });
    QObject::connect(confirm, &QPushButton::clicked, dialog, &QDialog::accept);
    QObject::connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);

    dialog->exec();
    return dialog;
}

QDialog *DialogFactory::faceAuth(QWidget *parent, const std::string &faceUrl)
{
    QDialog *dialog = createBaseDialog("实名认证（人脸识别）", parent);
    QVBoxLayout *layout = (QVBoxLayout *)dialog->layout();

    QLabel *qrLabel = new QLabel();
    QPixmap pixmap = Core::QrGenerator::generate(faceUrl);
    if (pixmap.isNull()) {
        qrLabel->setText("二维码生成失败，请检查网络或日志");
    } else {
        qrLabel->setPixmap(pixmap);
    }
    qrLabel->setAlignment(Qt::AlignCenter);

    QLabel *tipLabel = new QLabel(
        "请使用<b>手机 mybilibili App</b> 扫描下方二维码完成人脸认证。<br>"
        "认证完成后，请重新点击开始直播。");
    tipLabel->setWordWrap(true);
    tipLabel->setAlignment(Qt::AlignCenter);

    QPushButton *closeBtn = new QPushButton("我已完成认证");

    layout->addWidget(tipLabel);
    layout->addWidget(qrLabel);
    layout->addWidget(closeBtn);

    QObject::connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    QObject::connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);

    dialog->exec();
    return dialog;
}

QDialog *DialogFactory::roomSettings(QWidget *parent, const std::string &roomUrl, const std::string &currentTitle,
                    std::function<void(const std::string &title)> onApply)
{
    QDialog *dialog = createBaseDialog("更新直播间信息", parent);
    QVBoxLayout *layout = (QVBoxLayout *)dialog->layout();

    layout->addWidget(new QLabel(QString("直播间 ID: %1").arg(QString::fromStdString(roomUrl))));

    QLineEdit *titleInput = new QLineEdit(QString::fromStdString(currentTitle));
    QPushButton *confirmTitle = new QPushButton("确认");
    QHBoxLayout *titleRow = new QHBoxLayout();
    titleRow->addWidget(new QLabel("直播间标题:"));
    titleRow->addWidget(titleInput);
    titleRow->addWidget(confirmTitle);
    layout->addLayout(titleRow);

    QObject::connect(confirmTitle, &QPushButton::clicked, [=]() {
        if (onApply) {
            onApply(titleInput->text().trimmed().toUtf8().constData());
        }
        dialog->accept();
    });

    QObject::connect(dialog, &QDialog::finished, dialog, &QDialog::deleteLater);
    dialog->exec();
    return dialog;
}

} // namespace UI
