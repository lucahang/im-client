#include "mainwindow.h"
#include "imclientwrapper.h"
#include "chat_widget.h"
#include <QVBoxLayout>
#include <QSplitter>
#include <QListWidget>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(std::shared_ptr<IMClientWrapper> wrapper, QWidget* parent)
    : QMainWindow(parent), wrapper_(wrapper) {
    setWindowTitle("IM Client");

    resize(1000, 700);

    // 【新增】设置允许缩放到的最小尺寸，防止拉得太小导致界面重叠
    setMinimumSize(800, 500);

    // 创建联系人列表
    contactList_ = new QListWidget;
    contactList_->setMinimumWidth(100);

    // 创建聊天区域
    chatWidget_ = new ChatWidget(wrapper);

    // 分割布局
    auto splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(contactList_);
    splitter->addWidget(chatWidget_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);

    // 3. 【核心】设置初始大小比例（比如：左边 200px，右边 600px）
    splitter->setSizes(QList<int>() << 200 << 600);
    setCentralWidget(splitter);

    // 连接信号
    connect(wrapper_.get(), &IMClientWrapper::contactsReceived,
            this, &MainWindow::onContactsReceived);
    connect(wrapper_.get(), &IMClientWrapper::messageReceived,
            this, &MainWindow::onMessageReceived);
    connect(wrapper_.get(), &IMClientWrapper::historyReceived,
            this, &MainWindow::onHistoryReceived);
    connect(contactList_, &QListWidget::itemClicked,
            this, &MainWindow::onContactClicked);

    // 当断开连接时关闭窗口
    connect(wrapper_.get(), &IMClientWrapper::disconnected, this, &QMainWindow::close);

    // 默认禁用聊天区域（未选联系人）
    chatWidget_->setEnabled(false);
}

void MainWindow::onContactsReceived(const QList<QPair<QString, QString>>& contacts, const QList<bool>& isGroup) {
    contactList_->clear();
    for (int i = 0; i < contacts.size(); ++i) {
        QString displayName = contacts[i].second;
        auto item = new QListWidgetItem(displayName + (isGroup[i] ? " (Group)" : ""));
        item->setData(Qt::UserRole, contacts[i].first);   // 存储 ID
        item->setData(Qt::UserRole + 1, isGroup[i]);
        item->setBackground(QBrush(QColor(200, 200, 200)));
        contactList_->addItem(item);
    }
}

void MainWindow::onMessageReceived(const QString& sender, const QString& content, bool isGroup, const QString& groupId) {
    QString peerId = isGroup ? groupId : sender;
    QString display = QString("%1").arg(content);
    messageCache_[peerId].append({display, sender});
    //qDebug()<<"currentPeer_: "<<currentPeer_<<"    peerId: "<<peerId;
    //qDebug()<<"currentIsGroup_: "<<currentIsGroup_<<"    isGroup: "<<isGroup;
    //qDebug()<<"display: "<<display;

    // 如果当前选中的就是这个联系人，则更新聊天显示
    if (currentPeer_ == peerId && currentIsGroup_ == isGroup) {
        //qDebug()<<"update display...";
        chatWidget_->displayMessage(display);
    }
}

void MainWindow::onHistoryReceived(const QList<QPair<QString, QString>>& messages) {
    if (currentPeer_.isEmpty()) return;
    // 清空缓存并显示历史
    messageCache_[currentPeer_].clear();
    for (const auto& line : messages) {
        messageCache_[currentPeer_].append(line);
    }
    chatWidget_->displayMessages(messageCache_[currentPeer_]);
}

void MainWindow::onContactClicked(QListWidgetItem* item) {
    QString peerId = item->data(Qt::UserRole).toString();
    bool isGroup = item->data(Qt::UserRole + 1).toBool();
    currentPeer_ = peerId;
    currentIsGroup_ = isGroup;

    chatWidget_->setPeer(peerId, isGroup);
    chatWidget_->setEnabled(true);

    // // 如果有缓存消息，显示；否则请求历史（默认获取最近20条）
    // if (messageCache_.contains(peerId)) {
    //     chatWidget_->displayMessages(messageCache_[peerId]);
    // } else {
    //     wrapper_->doGetHistory(peerId, isGroup, 0, 20);
    // }
    
    wrapper_->doGetHistory(peerId, isGroup, 0, 20);
    // 清除未读（可选）
    wrapper_->doClearUnread(peerId, isGroup);
}