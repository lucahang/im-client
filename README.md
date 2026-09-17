# IM Client

基于 **C++17 + Qt6 + QML** 开发的即时通讯客户端，与 [im-server](https://github.com/lucahang/im-server) 配套使用。

客户端采用 C++ 负责网络通信与业务逻辑，Qt/QML 负责桌面 UI，通过 Qt Model/View 与 QML Context Property 实现消息数据和界面的绑定。

## Features

- 用户注册与登录
- TCP 长连接通信
- Boost.Asio 异步网络通信
- Protobuf 消息序列化
- 联系人列表
- 好友申请
- 同意/拒绝好友申请
- 删除好友
- 单聊消息
- 群聊消息协议
- 历史消息加载
- 未读消息处理
- 消息发送状态展示
- Qt Widgets + QML 混合 UI

## Architecture

```text
                    +---------------------+
                    |      QML UI         |
                    |                     |
                    | Chat List / Message |
                    +----------+----------+
                               |
                        Context Property
                               |
                    +----------v----------+
                    |     Qt Wrapper      |
                    |   Signal / Slot     |
                    +----------+----------+
                               |
                    +----------v----------+
                    |   C++ IM Client     |
                    |                     |
                    | Connect / Send      |
                    | Receive / Decode    |
                    +----------+----------+
                               |
                          Boost.Asio
                               |
                         TCP Connection
                               |
                    +----------v----------+
                    |     IM Server       |
                    +---------------------+
```

## Client Modules

### Network Layer

`IMClient` 负责与服务器建立 TCP 长连接，并使用 Boost.Asio 处理异步解析、连接、读取和写入。

主要流程：

```text
Resolve
   |
   v
Async Connect
   |
   v
Read 4-byte Length
   |
   v
Read Message Body
   |
   v
Protobuf Decode
   |
   v
Dispatch to UI / Business Layer
```

发送侧使用队列保证异步写操作的顺序，避免多个异步写操作同时操作 socket。

### Protocol

客户端与服务端共享 `proto/message.proto`，通过 Protobuf 生成 C++ 消息类型。

支持的主要命令包括：

```text
Register / Login
Single Message / Group Message
Get History / Load More History
Clear Unread
Get Contacts
Add Friend
Get Friend Requests
Response to Friend Request
Delete Friend
Heartbeat / Quit
```

### Qt / QML UI

客户端采用 Qt Widgets 与 QML 混合开发。

Qt Widgets 负责窗口、登录界面以及部分传统 UI 组件；聊天消息展示使用 QML，并通过 `QQuickWidget` 嵌入到 Qt Widgets 界面中。

消息数据通过 Qt Model 暴露给 QML：

```text
C++ MessageModel
       |
       | Context Property
       v
QML ListView
       |
       v
Message Delegate
```

### Message Model

`MessageModel` 负责维护聊天消息数据，并通过 Qt Model/View 机制通知 QML 更新界面。

主要用于：

- 消息列表管理
- 消息新增
- 消息状态更新
- QML 数据绑定
- 聊天记录展示

## Project Structure

```text
im-client/
├── CMakeLists.txt
├── include/
│   ├── im_client.h
│   ├── imclientwrapper.h
│   ├── message_model.h
│   └── ui/
│       ├── mainwindow.h
│       ├── login_dialog.h
│       ├── chat_widget.h
│       ├── friendrequest_dialog.h
│       └── friend_request_item.h
├── src/
│   ├── main.cpp
│   ├── im_client.cpp
│   ├── imclientwrapper.cpp
│   ├── message_model.cpp
│   ├── ui/
│   │   ├── mainwindow.cpp
│   │   ├── login_dialog.cpp
│   │   ├── chat_widget.cpp
│   │   ├── friendrequest_dialog.cpp
│   │   └── friend_request_item.cpp
│   └── others/
│       └── resources.qrc
├── proto/
│   └── message.proto
└── logs/
```

## Tech Stack

| 技术 | 用途 |
| --- | --- |
| C++17 | 核心开发语言 |
| Qt6 | 客户端框架 |
| Qt Widgets | 桌面窗口及传统 UI |
| Qt Quick / QML | 聊天界面与消息展示 |
| Boost.Asio | TCP 异步网络通信 |
| Protocol Buffers | 网络消息序列化 |
| CMake | 项目构建 |

## Build

### Environment

推荐环境：

- Windows 10/11
- Visual Studio 2022
- Qt 6.x
- CMake 3.16+
- Boost
- Protobuf

Linux 环境也可以根据 Qt 和依赖库配置进行编译。

### Compile

```bash
git clone https://github.com/lucahang/im-client.git
cd im-client

mkdir build
cd build
cmake ..
cmake --build . --config Release
```

构建完成后运行生成的 `im_client` 可执行文件。

> 客户端运行前需要保证 `im-server` 已启动，并根据实际服务器地址和端口进行配置。

## Server

对应的服务端项目：

https://github.com/lucahang/im-server

## Development Goals

- [ ] 完善自动重连机制
- [ ] 优化网络异常处理
- [ ] 消息历史分页加载优化
- [ ] 图片/文件消息
- [ ] 离线消息同步
- [ ] 桌面通知
- [ ] UI 主题与暗色模式
- [ ] 消息搜索

## Author

**luca jay**

C++ / Qt Developer