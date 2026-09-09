#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <memory>
class IMClientWrapper;

class AddFriendDialog : public QDialog{
    Q_OBJECT
public:
    explicit AddFriendDialog(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent=nullptr);
signals:

    //发送好友申请
    void addFriendRequest(
        const QString& username
    );
private slots:
    void onAddButtonClicked();
    void onSendAddFriendResult(bool success, int status, const QString& target_name);
private:
    std::shared_ptr<IMClientWrapper> wrapper_;
    QLineEdit* usernameEdit_;
    QPushButton* addButton_;

};

