#include "add_contacts_dialog.h"
#include "imclientwrapper.h"

#include <QMessageBox>
#include <QDebug>

AddFriendDialog::AddFriendDialog(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent)
    :QDialog(parent), wrapper_(wrapper){
    setWindowTitle(
        "Add Friend"
    );
    setFixedSize(
        300,
        150
    );
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
    addButton_ =
        new QPushButton(
            "Add",
            this
        );
    QVBoxLayout* layout =
        new QVBoxLayout(this);
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->addWidget(label);
    hLayout->addWidget(usernameEdit_);
    layout->addLayout(hLayout);
    layout->addWidget(addButton_);
    setLayout(layout);
    connect(
        addButton_,
        &QPushButton::clicked,
        this,
        &AddFriendDialog::
        onAddButtonClicked
    );

    connect(wrapper_.get(), &IMClientWrapper::sendAddFriendResult, this, &AddFriendDialog::onSendAddFriendResult);
}

void AddFriendDialog::onSendAddFriendResult(bool success, int status, const QString& target_name){
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

void AddFriendDialog::onAddButtonClicked(){
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
    wrapper_->doSendAddFriendReq(username,"");
    addButton_->setEnabled(false);
    qDebug()<<"AddButton clicked button";
}