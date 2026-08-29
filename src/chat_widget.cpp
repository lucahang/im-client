#include "chat_widget.h"
#include "imclientwrapper.h"
#include "message_model.h"

#include <QVBoxLayout>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlError>
#include <QDebug>
#include <QPair>

ChatWidget::ChatWidget(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent)
    : QWidget(parent), wrapper_(wrapper) {

    // 1. 创建模型
    messageModel_ = new MessageModel(this, wrapper);

    // 2. 创建 QQuickWidget 嵌入 QML 界面
    quickWidget_ = new QQuickWidget(this);
    quickWidget_->setResizeMode(QQuickWidget::SizeRootObjectToView);

    // 3. 注册 Model 到 QML 上下文
    quickWidget_->rootContext()->setContextProperty("messageModel", messageModel_);

    // 4. 【核心修改】监听 QML 加载状态，等它真正 Ready 了再拿 rootObj 并 connect
    connect(quickWidget_, &QQuickWidget::statusChanged, this, [this](QQuickWidget::Status status) {
        if (status == QQuickWidget::Ready) {
            QObject* rootObj = quickWidget_->rootObject();
            qDebug() << "✅ QML 真正 Ready 时的 rootObj 指针:" << rootObj;
            
            if (rootObj) {
                // 先断开可能重复的连接（安全起见），再建立连接
                disconnect(rootObj, SIGNAL(sendMessageRequested(QString)), this, SLOT(onSendMessageRequested(QString)));
                
                bool isConnected = connect(rootObj, SIGNAL(sendMessageRequested(QString)),
                                         this, SLOT(onSendMessageRequested(QString)));
                qDebug() << "✅ 重新绑定状态:" << isConnected;
            }
        } else if (status == QQuickWidget::Error) {
            qDebug() << "❌ QML 加载报错:";
            for (const auto& err : quickWidget_->errors()) {
                qDebug() << err.toString();
            }
        }
    });

    // 5. 加载 QML 文件
    quickWidget_->setSource(QUrl("qrc:/ui/chat_list_view.qml"));

    // 6. 布局设置
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(quickWidget_);
}

void ChatWidget::setPeer(const QString& peerId, bool isGroup) {
    currentPeer_ = peerId;
    currentIsGroup_ = isGroup;
    messageModel_->clear();
}

void ChatWidget::displayMessage(const QString& msg, bool isSelf) {
    messageModel_->appendMessage(msg, isSelf);
}

void ChatWidget::displayMessages(const QList<QPair<QString, QString>>& msgs) {
    messageModel_->setMessages(msgs);
}

void ChatWidget::onSendMessageRequested(const QString& content) {
    qDebug()<<"onSendMessageRequested function runs";
    if (content.isEmpty() || currentPeer_.isEmpty()) return;

    wrapper_->doSendMessage(currentPeer_, content, currentIsGroup_);
    displayMessage(content, true); // 自己发送的消息, isSelf = true
}