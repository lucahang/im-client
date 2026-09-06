#include "add_contacts_dialog.h"

#include <QMessageBox>


AddFriendDialog::AddFriendDialog(QWidget* parent)
    :QDialog(parent)
{


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



    layout->addWidget(label);

    layout->addWidget(usernameEdit_);

    layout->addWidget(addButton_);



    setLayout(layout);



    connect(
        addButton_,
        &QPushButton::clicked,
        this,
        &AddFriendDialog::
        onAddButtonClicked
    );



}



void AddFriendDialog::
onAddButtonClicked()
{

    QString username =
        usernameEdit_->text()
        .trimmed();



    if(username.isEmpty())
    {

        QMessageBox::warning(
            this,
            "Warning",
            "Username cannot be empty"
        );

        return;
    }



    /*
        这里发送信号

        后续连接网络层

        MainWindow
             |
             |
             v

        FriendManager

    */


    emit addFriendRequest(
        username
    );



}