#include "imclientwrapper.h"
#include <QMetaObject>
#include <QThread>
#include <iostream>
#include <QDebug>
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
    //qDebug("stopIoContext()");
    if (client_) {
        client_->Close();
    }
    ioc_.stop();
    if (ioThread_ && ioThread_->joinable()) {
        ioThread_->join();
    }
}

// status == 1 mean accept  
// status == 2 mean reject
void IMClientWrapper::doSendResponeToFriendReqsReq(
                const std::string& peer_id, int32_t status){
    boost::asio::post(ioc_, [this, peer_id, status]() {
        client_->SendResponeToFriendReqsReq(*currentUserId_, peer_id, status);
    });
}

void IMClientWrapper::doSendGetFriendReqsReq(){
    //qDebug()<<"doSendGetFriendReqsReq()";
    boost::asio::post(ioc_, [this]() {
        client_->SendGetFriendReqsReq();
    });
}

void IMClientWrapper::doDeleteFriendReq(const QString& peer_id){
    boost::asio::post(ioc_, [this, peer_id]() {
        client_->DeleteFriendReq(peer_id.toStdString());
    });
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

void IMClientWrapper::doSendAddFriendReq(const QString& target_name, const QString& sender_name, const QString& msg){
    std::string t = target_name.toStdString();
    std::string m = msg.toStdString();
    std::string s_n = sender_name.toStdString();
    boost::asio::post(ioc_, [this, t, s_n, m]() {
        client_->SendAddFriendReq(t, s_n, m);
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

void IMClientWrapper::doGetContacts(){
    boost::asio::post(ioc_, [this]() {
        client_->GetContacts();
    });
}

void IMClientWrapper::doDisconnect() {
    boost::asio::post(ioc_, [this]() {
        client_->Disconnect();
    });
}

// void IMClientWrapper::sendGetFriendReqsReq(){
//     boost::asio::post(ioc_, [this]() {
//         client_->SendGetFriendReqsReq();
//     });
// }

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
                setCurrentUserName(username);
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
        case im::CMD_GET_FRIEND_REQS_RES: {
            im::GetFriendRequestsResponse resp;
            if (resp.ParseFromString(msg.body())){
                QList<QVector<QString>> friendReqs;
                for (int i = 0; i < resp.requests_size(); ++i) {
                    const auto& r = resp.requests(i);
                    QVector<QString> vec;
                    vec.append(QString::fromStdString(r.from_user_id()));
                    vec.append(QString::fromStdString(r.sender_name()));
                    vec.append(QString::fromStdString(r.message()));
                    // vec.append(QString::fromStdString(std::to_string(r.status())));
                    friendReqs.append(vec);
                }
                emit friendReqsReceived(friendReqs);
            }
            break;
        }
        case im::CMD_SINGLE_MSG:{
            im::ChatMessage chat;
            if (chat.ParseFromString(msg.body())) {
                QString sender = QString::fromStdString(chat.sender());
                QString content = QString::fromStdString(chat.content());
                bool isGroup = (cmd == im::CMD_GROUP_MSG);
                QString groupId = "";
                qDebug() << "receive CMD_SINGLE_MSG";
                emit messageReceived(sender, content, isGroup, groupId);
            }
            break;
        }
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

        case im::CMD_ADD_FRIEND_RES: {
            im::AddFriendResponse resp;
            if (resp.ParseFromString(msg.body())) {
                bool ok = (resp.status() == 0);
                emit sendAddFriendResult(ok, resp.status(), QString::fromStdString(resp.msg()));
                // qDebug()<<"receive CMD_ADD_FRIEND_RES: "<<QString::fromStdString(resp.msg());
            }
            break;
        }

        case im::CMD_DELETE_FRIEND_RES: {
            im::DeleteFriendRes resp;
            if (resp.ParseFromString(msg.body())) {
                qDebug()<<"receive CMD_DELETE_FRIEND_RES: ";
                emit deleteFriendReceive(resp.status());
            }
            break;
        }
        case im::CMD_GET_HISTORY_RES: {
            im::HistoryResponse resp;
            if (resp.ParseFromString(msg.body())) {
                QList<QPair<QString, QString>> lines;
                for (const auto& m : resp.messages()) {
                    lines.append({QString("%1")
                                 .arg(QString::fromStdString(m.content())),
                                 QString::fromStdString(m.sender())});
                }
                emit historyReceived(lines);
            }
            break;
        }
        case im::CMD_GET_LOADMORE_HISTORY_RES: {
            im::HistoryResponse resp;
            qDebug()<<"receive CMD_GET_LOADMORE_HISTORY_RES";
            if (resp.ParseFromString(msg.body())) {
                QList<QPair<QString, QString>> lines;
                for (const auto& m : resp.messages()) {
                    lines.append({QString("%1")
                                 .arg(QString::fromStdString(m.content())),
                                 QString::fromStdString(m.sender())});
                }
                emit loadMoreHistoryReceived(lines);
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
        case im::CMD_RESPONE_TO_FRIEND_REQS_RES: {
            im::ResponseToFriendReqsRes resp;
            if (resp.ParseFromString(msg.body())) {
                emit clickReceive(resp.status());
            }
            break;
        }
        default:
            emit errorOccurred(QString("Unknown command: %1").arg(cmd));
            break;
    }
}