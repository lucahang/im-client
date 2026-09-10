#include "ui/friend_request_item.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFrame>

FriendRequestItem::FriendRequestItem(
    const QString& username,
    const QString& message,
    const QString& userId,
    QWidget* parent)
    : QWidget(parent)
    , username_(username)
    , message_(message)
    , userId_(userId)
{
    setupUi();
}

void FriendRequestItem::setupUi()
{
    setFixedHeight(80);

    // =========================
    // 头像
    // =========================

    avatarLabel_ = new QLabel(this);
    avatarLabel_->setFixedSize(50, 50);

    avatarLabel_->setText(
        username_.isEmpty()
            ? "?"
            : username_.left(1).toUpper()
    );

    avatarLabel_->setAlignment(Qt::AlignCenter);

    avatarLabel_->setStyleSheet(
        "QLabel {"
        "    background-color: #5B8FF9;"
        "    color: white;"
        "    border-radius: 25px;"
        "    font-size: 20px;"
        "    font-weight: bold;"
        "}"
    );


    // =========================
    // 用户信息
    // =========================

    usernameLabel_ = new QLabel(username_, this);

    usernameLabel_->setStyleSheet(
        "QLabel {"
        "    font-size: 15px;"
        "    font-weight: bold;"
        "    color: #222222;"
        "}"
    );


    messageLabel_ = new QLabel(message_, this);

    messageLabel_->setStyleSheet(
        "QLabel {"
        "    font-size: 12px;"
        "    color: #888888;"
        "}"
    );

    messageLabel_->setWordWrap(true);


    auto* infoLayout = new QVBoxLayout;
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(4);

    infoLayout->addWidget(usernameLabel_);
    infoLayout->addWidget(messageLabel_);


    // =========================
    // 同意按钮
    // =========================

    acceptButton_ = new QPushButton("同意", this);
    acceptButton_->setFixedSize(55, 30);

    acceptButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #07C160;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "}"
        ""
        "QPushButton:hover {"
        "    background-color: #06AD56;"
        "}"
    );


    // =========================
    // 拒绝按钮
    // =========================

    rejectButton_ = new QPushButton("拒绝", this);
    rejectButton_->setFixedSize(55, 30);

    rejectButton_->setStyleSheet(
        "QPushButton {"
        "    background-color: #EEEEEE;"
        "    color: #555555;"
        "    border: none;"
        "    border-radius: 5px;"
        "}"
        ""
        "QPushButton:hover {"
        "    background-color: #DDDDDD;"
        "}"
    );


    // =========================
    // 整体布局
    // =========================

    auto* mainLayout = new QHBoxLayout(this);

    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(12);

    mainLayout->addWidget(avatarLabel_);
    mainLayout->addLayout(infoLayout);
    mainLayout->addStretch();

    mainLayout->addWidget(acceptButton_);
    mainLayout->addWidget(rejectButton_);


    // =========================
    // 信号
    // =========================

    connect(
        acceptButton_,
        &QPushButton::clicked,
        this,
        &FriendRequestItem::acceptClicked
    );

    connect(
        rejectButton_,
        &QPushButton::clicked,
        this,
        &FriendRequestItem::rejectClicked
    );
}