#pragma once

#include <QWidget>
#include <memory>
class QLabel;
class QPushButton;
class IMClientWrapper;

class FriendRequestItem : public QWidget
{
    Q_OBJECT

public:
    explicit FriendRequestItem(
        const QString& username,
        const QString& message,
        const QString& userId,
        std::shared_ptr<IMClientWrapper>& wrapper,
        QWidget* parent = nullptr
    );

    void onAcceptClicked();
    void onRejectClicked();
    void onClickReceive(const int32_t status);
signals:

private:
    void setupUi();

private:
    std::shared_ptr<IMClientWrapper> wrapper_;

    QString username_;
    QString userId_;
    QString message_;

    QLabel* avatarLabel_;
    QLabel* usernameLabel_;
    QLabel* messageLabel_;

    QPushButton* acceptButton_;
    QPushButton* rejectButton_;
};