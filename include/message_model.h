#pragma once

#include "imclientwrapper.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QString>

struct ChatMessage {
    QString content;
    bool isSelf; // 可扩展字段，标识是否为自己发送
};

class MessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum MessageRoles {
        ContentRole = Qt::UserRole + 1,
        IsSelfRole
    };

    explicit MessageModel(QObject* parent = nullptr , std::shared_ptr<IMClientWrapper> wrapper= nullptr );

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void appendMessage(const QString& msg, bool isSelf = true);
    void setMessages(const QList<QPair<QString, QString>>& msgs);
    void clear();

private:
    QList<ChatMessage> messages_;
    std::optional<std::string> currentUserId_;
    std::shared_ptr<IMClientWrapper> wrapper_; 
};