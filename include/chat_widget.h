#pragma once
#include <QWidget>
#include <QQuickItem>
#include <memory>

class IMClientWrapper;
class MessageModel;
class QQuickWidget;

class ChatWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatWidget(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent = nullptr);

    void setPeer(const QString& peerId, bool isGroup);
    void displayMessage(const QString& msg, bool isSelf = false);
    void displayMessages(const QList<QPair<QString, QString>>& msgs);

private slots:
    void onSendMessageRequested(const QString& content);
    void onLoadMoreHistoryRequested(const int & cnt);

private:
    std::shared_ptr<IMClientWrapper> wrapper_;
    MessageModel* messageModel_ = nullptr;
    QQuickWidget* quickWidget_ = nullptr;

    QString currentPeer_;
    bool currentIsGroup_ = false;
};