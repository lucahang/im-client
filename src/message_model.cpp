#include "message_model.h"

#include <QDebug>

MessageModel::MessageModel(QObject* parent, std::shared_ptr<IMClientWrapper> wrapper ) 
        : QAbstractListModel(parent),  wrapper_(wrapper){
    currentUserId_ = wrapper_->getCurrentUserId();
}

int MessageModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return messages_.size();
}

QVariant MessageModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= messages_.size())
        return QVariant();

    const auto& item = messages_.at(index.row());
    switch (role) {
    case ContentRole:
        return item.content;
    case IsSelfRole:
        return item.isSelf;
    case StatusRole:
        return static_cast<int>(item.status);
    case MessageIdRole:
        return item.msg_id;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ContentRole] = "content";
    roles[IsSelfRole] = "isSelf";
    roles[StatusRole] = "status";
    roles[MessageIdRole] = "msg_id";
    return roles;
}

void MessageModel::appendMessage(const QString& msg, const QString& msg_unique_id, 
                                 bool isSelf, int32_t status) {
    beginInsertRows(QModelIndex(), messages_.size(), messages_.size());
    
    messages_.append({msg, isSelf, status, msg_unique_id});
    endInsertRows();
}

void MessageModel::setMsgStatus(const QString& peer_id, const QString& msg_unique_id, 
                                int32_t status){
    
    for (int i = messages_.size() - 1; i >= 0; --i) {
        qDebug() << "messages_[i]: "<<messages_[i].msg_id;
        qDebug() << "status: "<<status;
        if(msg_unique_id == messages_[i].msg_id){
            messages_[i].status = status;
            QModelIndex index = this->index(i, 0);
            emit dataChanged(index, index, {StatusRole});
            break;
        }
    }
}


void MessageModel::setMessages(const QList<QPair<QString, QString>>& msgs) {
    beginResetModel();
    messages_.clear();

    // 防御性检查：确保 currentUserId_ 有值
    QString currentUid = (currentUserId_) ? QString::fromStdString(*currentUserId_): "";

    for (int i = msgs.size() - 1; i >= 0; --i) {
        bool isSelf = (!currentUid.isEmpty() && msgs.at(i).second == currentUid);
        messages_.append({msgs.at(i).first, isSelf, 1});
    }
    endResetModel();
}

void MessageModel::clear() {
    beginResetModel();
    messages_.clear();
    endResetModel();
}