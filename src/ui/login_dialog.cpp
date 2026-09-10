#include "ui/login_dialog.h"
#include "imclientwrapper.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>

LoginDialog::LoginDialog(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent)
    : QDialog(parent), wrapper_(wrapper) {
    setWindowTitle("Login / Register");

    usernameEdit_ = new QLineEdit;
    usernameEdit_->setPlaceholderText("Username");
    passwordEdit_ = new QLineEdit;
    passwordEdit_->setPlaceholderText("Password");
    passwordEdit_->setEchoMode(QLineEdit::Password);

    loginBtn_ = new QPushButton("Login");
    registerBtn_ = new QPushButton("Register");

    auto layout = new QVBoxLayout;
    layout->addWidget(new QLabel("Username:"));
    layout->addWidget(usernameEdit_);
    layout->addWidget(new QLabel("Password:"));
    layout->addWidget(passwordEdit_);

    auto hLayout = new QHBoxLayout;
    hLayout->addWidget(loginBtn_);
    hLayout->addWidget(registerBtn_);
    layout->addLayout(hLayout);

    setLayout(layout);

    connect(loginBtn_, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(registerBtn_, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
    connect(wrapper_.get(), &IMClientWrapper::loginResult, this, &LoginDialog::onLoginResult);
    connect(wrapper_.get(), &IMClientWrapper::registerResult, this, &LoginDialog::onRegisterResult);
}
void LoginDialog::onRegisterResult(bool success, int status, const QString& errorMsg) {
    // 恢复按钮点击
    loginBtn_->setEnabled(true);
    registerBtn_->setEnabled(true);

    if (success) {
        QMessageBox::information(this, "Success", "Registration successful! You can now log in.");
    } else {
        // 提示错误原因（如：用户名已存在、密码格式不合规等）
        QMessageBox::warning(this, "Registration Failed", errorMsg);
    }
}
void LoginDialog::onLoginClicked() {
    QString u = usernameEdit_->text().trimmed();
    QString p = passwordEdit_->text().trimmed();
    if (u.isEmpty() || p.isEmpty()) {
        QMessageBox::warning(this, "Error", "Username and password cannot be empty.");
        return;
    }
    wrapper_->doLogin(u, p);
    loginBtn_->setEnabled(false);
    registerBtn_->setEnabled(false);
}

void LoginDialog::onRegisterClicked() {
    QString u = usernameEdit_->text().trimmed();
    QString p = passwordEdit_->text().trimmed();
    if (u.isEmpty() || p.isEmpty()) {
        QMessageBox::warning(this, "Error", "Username and password cannot be empty.");
        return;
    }
    wrapper_->doRegister(u, p);
    loginBtn_->setEnabled(false);
    registerBtn_->setEnabled(false);
}

void LoginDialog::onLoginResult(bool success, int status, const QString& userId, const QString& username) {
    loginBtn_->setEnabled(true);
    registerBtn_->setEnabled(true);
    //qDebug() << success << status << userId << username;
    if (success) {
        accept();  // 关闭对话框，主窗口将显示
    } else {
        QString msg = (status == 1) ? "Wrong password" :
                      (status == 2) ? "User not found" : "Unknown error";
        QMessageBox::warning(this, "Login Failed", msg);
    }
}