#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>


class AddFriendDialog : public QDialog{
    Q_OBJECT
public:
    explicit AddFriendDialog(QWidget* parent=nullptr);
signals:

    //发送好友申请
    void addFriendRequest(
        const QString& username
    );



private slots:


    void onAddButtonClicked();



private:

    QLineEdit* usernameEdit_;

    QPushButton* addButton_;

};

