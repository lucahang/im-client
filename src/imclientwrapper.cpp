#include "imclientwrapper.h"
#include <QMetaObject>
#include <QThread>
#include <iostream>

IMClientWrapper::IMClientWrapper(const std::string& host, uint16_t port, QObject* parent)
    : QObject(parent), host_(host), port_(port) {
    // 设置网络回调
    auto onRecv = [this](const im::Message& msg) {
        // 此 lambda 运行在 io_context 线程，必须通过 Qt 队列转到主线程
        QMetaObject::invokeMethod(this, [this, msg]() {
            onNetworkMessage(msg);
        }, Qt::QueuedConnection);
    };

    client_ = std::make_shared<IMClient>(ioc_, host_, port_, onRecv);
    startIoContext();

    // 连接服务器（非阻塞，io_context 线程执行）
    boost::asio::post(ioc_, [this]() {
        client_->Connect();
    });
}

IMClientWrapper::~IMClientWrapper() {
    stopIoContext();
}

void IMClientWrapper::startIoContext() {
    ioThread_ = std::make_unique<std::thread>([this]() {
        try {
            ioc_.run();
        } catch (const std::exception& e) {
            std::cerr << "io_context error: " << e.what() << std::endl;
        }
    });
}

void IMClientWrapper::stopIoContext() {
    if (client_) {
        client_->Close();
    }
    ioc_.stop();
    if (ioThread_ && ioThread_->joinable()) {
        ioThread_->join();
    }
}

// ---------- 公开接口（线程安全，向 io_context 投递任务）----------
void IMClientWrapper::doRegister(const QString& username, const QString& password) {
    std::string u = username.toStdString();
    std::string p = password.toStdString();
    boost::asio::post(ioc_, [this, u, p]() {
        client_->RegisterUser(u, p);
    });
}

void IMClientWrapper::doLogin(const QString& username, const QString& password) {
    std::string u = username.toStdString();
    std::string p = password.toStdString();
    boost::asio::post(ioc_, [this, u, p]() {
        client_->LoginUser(u, p);
    });
}

void IMClientWrapper::doSendMessage(const QString& receiverId, const QString& content, bool isGroup) {
    std::string rid = receiverId.toStdString();
    std::string cnt = content.toStdString();
    boost::asio::post(ioc_, [this, rid, cnt, isGroup]() {
        if (isGroup)
            client_->SendGroupMsg(rid, cnt);
        else
            client_->SendSingleMsg(rid, cnt);
    });
}

void IMClientWrapper::doGetHistory(const QString& peerId, bool isGroup, int64_t start, int32_t count) {
    std::string pid = peerId.toStdString();
    boost::asio::post(ioc_, [this, pid, isGroup, start, count]() {
        client_->GetHistory(pid, isGroup, start, count);
    });
}

void IMClientWrapper::doClearUnread(const QString& peerId, bool isGroup) {
    std::string pid = peerId.toStdString();
    boost::asio::post(ioc_, [this, pid, isGroup]() {
        client_->ClearUnread(pid, isGroup);
    });
}

void IMClientWrapper::doDisconnect() {
    boost::asio::post(ioc_, [this]() {
        client_->Disconnect();
    });
}

// ---------- 网络消息处理（在主线程执行）----------
void IMClientWrapper::onNetworkMessage(const im::Message& msg) {
    int cmd = msg.header().cmd();
    switch (cmd) {
        case im::CMD_REGISTER_RES: {
            im::RegisterResponse resp;
            if (resp.ParseFromString(msg.body())) {
                bool ok = (resp.status() == 0);
                emit registerResult(ok, resp.status(), ok ? "Success" : "Username exists");
            }
            break;
        }
        case im::CMD_LOGIN_RES: {
            im::LoginResponse resp;
            if (resp.ParseFromString(msg.body())) {
                bool ok = (resp.status() == 0);
                QString userId = ok ? QString::fromStdString(resp.user_id()) : "";
                QString username = ok ? QString::fromStdString(resp.username()) : "";
                emit loginResult(ok, resp.status(), userId, username);
                if (ok) {
                    currentUserId_ = resp.user_id();
                    // 登录成功后自动请求联系人列表
                    boost::asio::post(ioc_, [this]() {
                        client_->GetContacts();
                    });
                }
            }
            break;
        }
        case im::CMD_GET_CONTACTS_RES: {
            // 假设 proto 中定义了 GetContactsResponse
            im::ContactResponse resp;
            if (resp.ParseFromString(msg.body())) {
                QList<QPair<QString, QString>> contacts;
                QList<bool> isGroupList;
                for (int i = 0; i < resp.contacts_size(); ++i) {
                    const auto& c = resp.contacts(i);
                    contacts.append(qMakePair(QString::fromStdString(c.user_id()),
                                              QString::fromStdString(c.alias())));
                    isGroupList.append(c.is_group());
                }
                emit contactsReceived(contacts, isGroupList);
            }
            break;
        }
        case im::CMD_SINGLE_MSG:
        case im::CMD_GROUP_MSG: {
            im::ChatMessage chat;
            if (chat.ParseFromString(msg.body())) {
                QString sender = QString::fromStdString(chat.sender());
                QString content = QString::fromStdString(chat.content());
                bool isGroup = (cmd == im::CMD_GROUP_MSG);
                QString groupId = isGroup ? QString::fromStdString(chat.group_id()) : "";
                emit messageReceived(sender, content, isGroup, groupId);
            }
            break;
        }
        case im::CMD_GET_HISTORY_RES: {
            im::HistoryResponse resp;
            if (resp.ParseFromString(msg.body())) {
                QList<QString> lines;
                for (const auto& m : resp.messages()) {
                    lines.append(QString("[%1] %2: %3")
                                 .arg(m.msg_id())
                                 .arg(QString::fromStdString(m.sender()))
                                 .arg(QString::fromStdString(m.content())));
                }
                emit historyReceived(lines);
            }
            break;
        }
        case im::CMD_CLEAR_UNREAD_RES: {
            emit clearUnreadResult(true);
            break;
        }
        case im::CMD_QUIT_RES: {
            emit disconnected();
            break;
        }
        default:
            emit errorOccurred(QString("Unknown command: %1").arg(cmd));
            break;
    }
}