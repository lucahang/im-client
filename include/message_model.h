#pragma once

#include "imclientwrapper.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QString>

struct ChatMessage {
    QString content;
    bool isSelf; // 可扩展字段，标识是否为自己发送
    int32_t status;
    QString msg_id;
    QString peerName;
};

class MessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum MessageRoles {
        ContentRole = Qt::UserRole + 1,
        MessageIdRole,
        IsSelfRole,
        StatusRole,
        PeerNameRole
    };

    explicit MessageModel(QObject* parent = nullptr , std::shared_ptr<IMClientWrapper> wrapper= nullptr );

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    void setPeerName(const QString& peerName){peerName_ = peerName;}
    void appendMessage(const QString& msg, const QString& msg_unique_id, 
                    bool isSelf = true, int32_t status = 0, const QString& peerName = "");
    void setMessages(const QList<QPair<QString, QString>>& msgs);
    void setMsgStatus(const QString& peer_id, const QString& msg_unique_id, int32_t status);
    void clear();

private:
    QList<ChatMessage> messages_;
    std::optional<std::string> currentUserId_;
    std::shared_ptr<IMClientWrapper> wrapper_; 
    QString peerName_;
};