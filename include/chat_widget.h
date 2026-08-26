#pragma once
#include <QWidget>
#include <memory>
class IMClientWrapper;
class QTextEdit;
class QLineEdit;
class QPushButton;

class ChatWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatWidget(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent = nullptr);

    void setPeer(const QString& peerId, bool isGroup);
    void displayMessage(const QString& msg);
    void displayMessages(const QList<QString>& msgs);

private slots:
    void onSendClicked();

private:
    std::shared_ptr<IMClientWrapper> wrapper_;
    QTextEdit* textDisplay_;
    QLineEdit* inputEdit_;
    QPushButton* sendBtn_;
    QString currentPeer_;
    bool currentIsGroup_ = false;
};