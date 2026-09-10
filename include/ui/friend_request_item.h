#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

class FriendRequestItem : public QWidget
{
    Q_OBJECT

public:
    explicit FriendRequestItem(
        const QString& username,
        const QString& message,
        const QString& userId,
        QWidget* parent = nullptr
    );

signals:
    void acceptClicked();
    void rejectClicked();

private:
    void setupUi();

private:
    QString username_;
    QString userId_;
    QString message_;

    QLabel* avatarLabel_;
    QLabel* usernameLabel_;
    QLabel* messageLabel_;

    QPushButton* acceptButton_;
    QPushButton* rejectButton_;
};