import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: root
    // 拉取更多历史消息的信号
    signal loadMoreHistoryRequested(int cnt)
    // 暴露给 C++ 调用的信号
    signal sendMessageRequested(string content)

    

    ColumnLayout {
        anchors.fill: parent
        spacing: 8
        

        Timer {
            id: scrollTimer

            interval: 50

            onTriggered: {
                listView.positionViewAtEnd()
            }
        }
        // 消息展示列表
        ListView {
            id: listView

            property bool firstLoad: true


            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: messageModel // C++ 端设置的 Context Property
            spacing: 30

            property int lastCount: 0
            property bool isLoadingMore: false
            
            // 1. 数据增加时，延迟一帧滚动到底部（等待 Delegate 尺寸计算完毕）
            onCountChanged: {
                //console.log("model.peerName: {}" + model.peerName) 
                if (firstLoad && count > 0) {
                    firstLoad = false
                    scrollTimer.start()
                }

                if(count == lastCount+1){
                    scrollTimer.start()
                }
                if(count == 0 ){
                    firstLoad = true
                }
                lastCount = count
            }

            // 2. 窗口高度变化时，同样自动置底
            onHeightChanged: {
                Qt.callLater(function() {
                    listView.positionViewAtEnd()
                })
            } // <--- 修复：这里之前漏掉了闭合括号

            onContentYChanged: {
                // contentY <= 0 说明已经滑动到了最顶端
                if (contentY <= 0 && !isLoadingMore && count > 0) {
                    console.log("触顶，触发加载更多历史消息...");
                    isLoadingMore = true
                    
                    // 记录加载前列表的实际高度，用于加载完成后维持视口位置
                    var oldContentHeight = listView.contentHeight
                    
                    // 触发信号通知 C++
                    root.loadMoreHistoryRequested(count)

                    // 使用 Qt.callLater 在 C++ 插入完数据并重新布局后调整 contentY
                    Qt.callLater(function() {
                        var newContentHeight = listView.contentHeight
                        // 修正 contentY，保持视野停留在加载前的顶部位置
                        listView.contentY = newContentHeight - oldContentHeight
                        isLoadingMore = false
                    })
                }
            }


            delegate: Rectangle {
                width: listView.width
                height: contentText.implicitHeight + 36
                color: "transparent"
                
                //avatar
                Rectangle {
                    id: avatar

                    width: 40
                    height: 40

                    anchors.top: parent.top
                    anchors.topMargin: 8

                    anchors.left: model.isSelf ? undefined : parent.left
                    anchors.right: model.isSelf ? parent.right : undefined

                    anchors.leftMargin: 8
                    anchors.rightMargin: 8

                    color: "#cccccc"
                    radius: 6

                    Text {
                        anchors.centerIn: parent
                        // 这里改成你的用户名字段
                        text: model.isSelf
                            ? "me"
                            : model.peerName;

                        font.pixelSize: 12
                        color: "#333333"
                    }
                }
                
                //message
                Rectangle {
                    id: messageBubble

                    anchors.top: parent.top
                    anchors.topMargin: 8

                    anchors.bottom: parent.bottom

                    // 对方消息：头像右边开始
                    anchors.left: model.isSelf ? undefined : avatar.right

                    // 自己消息：头像左边结束
                    anchors.right: model.isSelf ? avatar.left : undefined
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10

                    width: Math.min(contentText.implicitWidth + 24, listView.width * 0.65)
                    color: model.isSelf ? "#95ec69" : "#d7c8c894"
                    height: contentText.implicitHeight + 16
                    radius: 6

                    Text {
                        id: contentText
                        anchors.fill: parent
                        anchors.margins: 8
                        text: model.content
                        wrapMode: Text.Wrap
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 20
                    }

                    Text {
                        id: statusText

                        anchors.left: parent.left
                        anchors.top: parent.bottom

                        anchors.rightMargin: 8
                        anchors.bottomMargin: 4


                        text: {
                            if (model.isSelf) {
                                if(model.status == 1){
                                    return "sent"
                                }
                                else if(model.status == 0){
                                    return "sending"
                                }
                            } else {
                                return ""
                            }
                        }


                        font.pixelSize: 15
                        //font.bold: true
                        color: "#1d1c22"
                    }
                }
            }
            footer: Item {
                width: listView.width
                height: 30
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
