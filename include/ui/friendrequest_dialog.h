#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QList>
#include <QVector>
#include <memory>
class IMClientWrapper;

class FriendRequestDialog : public QDialog{
    Q_OBJECT
public:
    explicit FriendRequestDialog(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent=nullptr);
signals:

    //发送好友申请
    void addFriendRequest(
        const QString& username
    );

private slots:
    void addRequestItem(const QString& username, const QString& message, const QString& userId);
    void onFriendReqsReceived(const QList<QVector<QString>>& messages);
    void onAddButtonClicked();
    void onSendAddFriendResult(bool success, int status, const QString& target_name);
private:
    std::shared_ptr<IMClientWrapper> wrapper_;
    QLineEdit* usernameEdit_;
    QPushButton* addButton_;

    QListWidget* requestList_;

};
