#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <memory>
class IMClientWrapper;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent = nullptr);

private slots:
    void onLoginClicked();
    void onRegisterClicked();
    void onLoginResult(bool success, int status, const QString& userId, const QString& username);
    void onRegisterResult(bool success, int status, const QString& errorMsg);

private:
    std::shared_ptr<IMClientWrapper> wrapper_;
    QLineEdit* usernameEdit_;
    QLineEdit* passwordEdit_;
    QPushButton* loginBtn_;
    QPushButton* registerBtn_;
};