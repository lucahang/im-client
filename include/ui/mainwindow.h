#pragma once
#include <QMainWindow>
#include <memory>
#include <QList>
#include <QPair>
#include <QVector>
#include <QPushButton>
#include <QString>
#include <QListWidgetItem>
class IMClientWrapper;
class ChatWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent = nullptr);
    ~MainWindow();
private slots:
    void showAddFriendDialog();
    void onContactsReceived(const QList<QPair<QString, QString>>& contacts, const QList<bool>& isGroup);
    void onMessageReceived(const QString& sender, const QString& content, bool isGroup, const QString& groupId);
    void onHistoryReceived(const QList<QPair<QString, QString> >& messages);
    void onLoadMoreHistoryReceived(const QList<QPair<QString, QString>>& messages);
    //void onLoadMoreHistoryRequested(const QList<QPair<QString, QString> >& messages);
    void onContactClicked(QListWidgetItem* item);
    void onDeleteFriendReceive(const int32_t status);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    std::shared_ptr<IMClientWrapper> wrapper_;
    QListWidget* contactList_;
    ChatWidget* chatWidget_;
    QMap<QString, QList<QPair<QString, QString> >> messageCache_;   // peerId -> messages
    QString currentPeer_;
    bool currentIsGroup_;
    QPushButton* addButton_;
    QString currentUserName_;
};