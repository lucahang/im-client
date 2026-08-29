#pragma once
#include <QMainWindow>
#include <memory>
#include <QList>
#include <QPair>
#include <QString>
#include <QListWidgetItem>
class IMClientWrapper;
class ChatWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent = nullptr);

private slots:
    void onContactsReceived(const QList<QPair<QString, QString>>& contacts, const QList<bool>& isGroup);
    void onMessageReceived(const QString& sender, const QString& content, bool isGroup, const QString& groupId);
    void onHistoryReceived(const QList<QPair<QString, QString> >& messages);
    void onLoadMoreHistoryReceived(const QList<QPair<QString, QString>>& messages);
    //void onLoadMoreHistoryRequested(const QList<QPair<QString, QString> >& messages);
    void onContactClicked(QListWidgetItem* item);

private:
    std::shared_ptr<IMClientWrapper> wrapper_;
    QListWidget* contactList_;
    ChatWidget* chatWidget_;
    QMap<QString, QList<QPair<QString, QString> >> messageCache_;   // peerId -> messages
    QString currentPeer_;
    bool currentIsGroup_;
};