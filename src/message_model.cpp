#include "message_model.h"

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
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[ContentRole] = "content";
    roles[IsSelfRole] = "isSelf";
    return roles;
}

void MessageModel::appendMessage(const QString& msg, bool isSelf) {
    beginInsertRows(QModelIndex(), messages_.size(), messages_.size());
    messages_.append({msg, isSelf});
    endInsertRows();
}


void MessageModel::setMessages(const QList<QPair<QString, QString>>& msgs) {
    beginResetModel();
    messages_.clear();

    // 防御性检查：确保 currentUserId_ 有值
    QString currentUid = (currentUserId_) ? QString::fromStdString(*currentUserId_): "";

    for (int i = msgs.size() - 1; i >= 0; --i) {
        bool isSelf = (!currentUid.isEmpty() && msgs.at(i).second == currentUid);
        messages_.append({msgs.at(i).first, isSelf});
    }
    endResetModel();
}

void MessageModel::clear() {
    beginResetModel();
    messages_.clear();
    endResetModel();
}