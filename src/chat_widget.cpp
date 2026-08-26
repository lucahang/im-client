#include "chat_widget.h"
#include "imclientwrapper.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>

ChatWidget::ChatWidget(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent)
    : QWidget(parent), wrapper_(wrapper) {
    textDisplay_ = new QTextEdit;
    textDisplay_->setReadOnly(true);
    inputEdit_ = new QLineEdit;
    inputEdit_->setPlaceholderText("Type message...");
    sendBtn_ = new QPushButton("Send");

    auto layout = new QVBoxLayout;
    layout->addWidget(textDisplay_);
    auto hLayout = new QHBoxLayout;
    hLayout->addWidget(inputEdit_);
    hLayout->addWidget(sendBtn_);
    layout->addLayout(hLayout);
    setLayout(layout);

    connect(sendBtn_, &QPushButton::clicked, this, &ChatWidget::onSendClicked);
    connect(inputEdit_, &QLineEdit::returnPressed, this, &ChatWidget::onSendClicked);
}

void ChatWidget::setPeer(const QString& peerId, bool isGroup) {
    currentPeer_ = peerId;
    currentIsGroup_ = isGroup;
    textDisplay_->clear();
}

void ChatWidget::displayMessage(const QString& msg) {
    textDisplay_->append(msg);
}

void ChatWidget::displayMessages(const QList<QString>& msgs) {
    textDisplay_->clear();
    for (const QString& m : msgs) {
        textDisplay_->append(m);
    }
}

void ChatWidget::onSendClicked() {
    QString content = inputEdit_->text().trimmed();
    if (content.isEmpty() || currentPeer_.isEmpty()) return;
    wrapper_->doSendMessage(currentPeer_, content, currentIsGroup_);
    inputEdit_->clear();
}