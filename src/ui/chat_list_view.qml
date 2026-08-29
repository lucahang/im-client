import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root

    // 暴露给 C++ 调用的信号
    signal sendMessageRequested(string content)

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        // 消息展示列表
        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: messageModel // C++ 端设置的 Context Property
            spacing: 8

            // 1. 数据增加时，延迟一帧滚动到底部（等待 Delegate 尺寸计算完毕）
            onCountChanged: {
                Qt.callLater(function() {
                    listView.positionViewAtEnd()
                })
            }

            // 2. 窗口高度变化时，同样自动置底
            onHeightChanged: {
                Qt.callLater(function() {
                    listView.positionViewAtEnd()
                })
            } // <--- 修复：这里之前漏掉了闭合括号

            delegate: Rectangle {
                width: listView.width
                height: contentText.implicitHeight + 16
                color: "transparent"

                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    // 区分发送者与接收者的左右靠齐（通过 isSelf）
                    anchors.right: model.isSelf ? parent.right : undefined
                    anchors.left: model.isSelf ? undefined : parent.left
                    anchors.margins: 8

                    width: Math.min(contentText.implicitWidth + 24, listView.width * 0.7)
                    color: model.isSelf ? "#95ec69" : "#d7c8c894"
                    radius: 6

                    Text {
                        id: contentText
                        anchors.fill: parent
                        anchors.margins: 8
                        text: model.content
                        wrapMode: Text.Wrap
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 14
                    }
                }
            }
        } // <--- 修复：删除了底部多余的 onCountChanged 重复块

        // 底部输入区域
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            spacing: 8

            TextField {
                id: inputEdit
                Layout.fillWidth: true
                placeholderText: "Type message..."
                onAccepted: sendBtn.clicked()
            }

            Button {
                id: sendBtn
                text: "Send"
                onClicked: {
                    var txt = inputEdit.text.trim()
                    if (txt.length > 0) {
                        console.log("Text to send:", txt);
                        root.sendMessageRequested(txt)
                        inputEdit.clear()
                    }
                }
            }
        }
    }
}