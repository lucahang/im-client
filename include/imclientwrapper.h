#pragma once
#include <QObject>
#include <memory>
#include <thread>
#include <optional>
#include "im_client.h"

class IMClientWrapper : public QObject {
    Q_OBJECT
public:
    explicit IMClientWrapper(const std::string& host, uint16_t port, QObject* parent = nullptr);
    ~IMClientWrapper();

    // 供 GUI 调用的接口（线程安全，内部会投递到 io_context）
    void doRegister(const QString& username, const QString& password);
    void doLogin(const QString& username, const QString& password);
    void doSendMessage(const QString& receiverId, const QString& content, bool isGroup);
    void doGetContacts();
    void doGetHistory(const QString& peerId, bool isGroup, int64_t start, int32_t count);
    void doClearUnread(const QString& peerId, bool isGroup);
    void doDisconnect();

signals:
    // 这些信号在主线程中触发（通过 Qt 自动排队）
    void registerResult(bool success, int status, const QString& message);
    void loginResult(bool success, int status, const QString& userId, const QString& username);
    void contactsReceived(const QList<QPair<QString, QString>>& contacts, const QList<bool>& isGroup);
    void messageReceived(const QString& sender, const QString& content, bool isGroup, const QString& groupId);
    void historyReceived(const QList<QString>& messages);   // 实际可定义结构
    void clearUnreadResult(bool success);
    void disconnected();
    void errorOccurred(const QString& error);

private slots:
    void onNetworkMessage(const im::Message& msg);   // 在主线程执行

private:
    std::shared_ptr<IMClient> client_;
    std::unique_ptr<std::thread> ioThread_;
    boost::asio::io_context ioc_;
    std::string host_;
    uint16_t port_;
    std::optional<std::string> currentUserId_;

    void startIoContext();
    void stopIoContext();
};