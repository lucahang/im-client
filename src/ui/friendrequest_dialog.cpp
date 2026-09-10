#include "ui/friendrequest_dialog.h"
#include "ui/friend_request_item.h"
#include "imclientwrapper.h"

#include <QMessageBox>
#include <QHBoxLayout>
#include <QDebug>

FriendRequestDialog::FriendRequestDialog(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent)
    :QDialog(parent), wrapper_(wrapper){

    requestList_ = new QListWidget(this);
    
    // addRequestItem(
    //     "Alice",
    //     "你好，我想添加你为好友"
    // );
    
    // addRequestItem(
    //     "Bob",
    //     "你好，可以加个好友吗？"
    // );

    // addRequestItem(
    //     "Charlie",
    //     "我是你的同学"
    // );



    setWindowTitle("Add Friend");

    setFixedSize(500, 600);


    // ==================================================
    // 主布局
    // ==================================================

    auto* mainLayout = new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        20,
        20,
        20,
        20
    );

    mainLayout->setSpacing(15);


    // ==================================================
    // 输入用户名区域
    // ==================================================

    QLabel* label =
        new QLabel(
            "Username:",
            this
        );


    usernameEdit_ =
        new QLineEdit(this);

    usernameEdit_
        ->setPlaceholderText(
            "Enter username"
        );


    // 添加按钮

    addButton_ =
        new QPushButton(
            "Add",
            this
        );


    // 水平布局

    auto* hLayout =
        new QHBoxLayout;

    hLayout->addWidget(label);

    hLayout->addWidget(
        usernameEdit_,
        1
    );

    hLayout->addWidget(
        addButton_
    );


    // 添加到主布局

    mainLayout->addLayout(
        hLayout
    );


    // ==================================================
    // 分割线
    // ==================================================

    auto* line =
        new QFrame(this);

    line->setFrameShape(
        QFrame::HLine
    );

    line->setFrameShadow(
        QFrame::Plain
    );

    line->setStyleSheet(
        "color: #040000;"
    );

    mainLayout->addWidget(
        line
    );


    // ==================================================
    // 好友申请标题
    // ==================================================

    auto* requestTitle =
        new QLabel(
            "Friend Requests",
            this
        );

    requestTitle->setStyleSheet(
        "QLabel {"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    color: #333333;"
        "}"
    );

    mainLayout->addWidget(
        requestTitle
    );


    // ==================================================
    // 好友申请列表
    // ==================================================

    

    requestList_->setSpacing(
        5
    );

    requestList_->setFrameShape(
        QFrame::NoFrame
    );

    requestList_->setStyleSheet(
        "QListWidget {"
        "    background-color: #F7F7F7;"
        "    border: none;"
        "}"
        ""
        "QListWidget::item {"
        "    background-color: white;"
        "    border-radius: 6px;"
        "}"
        ""
        "QListWidget::item:selected {"
        "    background-color: white;"
        "}"
    );


    // 让 QListWidget 占据剩余空间

    mainLayout->addWidget(
        requestList_,
        1
    );

    connect(addButton_,&QPushButton::clicked,
        this,&FriendRequestDialog::onAddButtonClicked);  
    connect(wrapper_.get(), &IMClientWrapper::sendAddFriendResult, 
        this, &FriendRequestDialog::onSendAddFriendResult);
    connect(wrapper_.get(), &IMClientWrapper::friendReqsReceived,
        this, &FriendRequestDialog::onFriendReqsReceived);
}

void FriendRequestDialog::onFriendReqsReceived(const QList<QVector<QString>>& messages){
    for(const auto& message : messages){
        addRequestItem(
            message[1],
            message[2],
            message[0]
        );
    }
}

void FriendRequestDialog::addRequestItem(
    const QString& username,
    const QString& message,
    const QString& userId){
    auto* item = new QListWidgetItem(requestList_);

    auto* widget = new FriendRequestItem(
        username,
        message,
        userId,
        this
    );

    item->setSizeHint(
        QSize(
            0,
            widget->sizeHint().height()
        )
    );

    requestList_->addItem(item);

    requestList_->setItemWidget(
        item,
        widget
    );


    // ===============================
    // 同意
    // ===============================

    // connect(
    //     widget,
    //     &FriendRequestItem::acceptClicked,
    //     this,
    //     [this, item]()
    //     {
    //         int row =
    //             requestList_->row(item);

    //         delete requestList_->takeItem(row);
    //     }
    // );


    // ===============================
    // 拒绝
    // ===============================

    // connect(
    //     widget,
    //     &FriendRequestItem::rejectClicked,
    //     this,
    //     [this, item]()
    //     {
    //         int row =
    //             requestList_->row(item);

    //         delete requestList_->takeItem(row);
    //     }
    // );
}

void FriendRequestDialog::onSendAddFriendResult(bool success, int status, const QString& target_name){
    // 恢复按钮点击
    addButton_->setEnabled(true);

    if (success) {
        QString msg = QString("YOU SENT A FRIEND REQUEST TO %1 SUCCESSFULLY").arg(target_name);
        QMessageBox::information(this, "Success", msg);
    } else {
        if(status == 1 ){
            QMessageBox::warning(this, "Failed", "already sent friend request before");
        }
        else if(status == 2){
            QMessageBox::warning(this, "Failed", "couldn't find user friend request failed");
        }
        else if(status == 3){
            QMessageBox::warning(this, "Failed", "You couldn't send friend request to yourself");
        }
        else {
            QMessageBox::warning(this, "Failed", "sent friend request failed");
        }
    }
}

void FriendRequestDialog::onAddButtonClicked(){
    QString username =
        usernameEdit_->text()
        .trimmed();
    if(username.isEmpty()){
        QMessageBox::warning(
            this,
            "Warning",
            "Username cannot be empty"
        );
        return;
    }
    wrapper_->doSendAddFriendReq(username, QString::fromStdString(wrapper_->getCurrentUserName()) ,"");
    addButton_->setEnabled(false);
    qDebug()<<"AddButton clicked button";
}