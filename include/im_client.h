#pragma once
#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <string>
#include <mutex>
#include <functional>
#include <optional>
#include "message.pb.h"

class Codec {
public:
    static std::string Encode(const im::Message& msg);
    static std::optional<im::Message> Decode(const char* data, size_t len);
};

class IMClient : public std::enable_shared_from_this<IMClient> {
public:
    using ReceiveCallback = std::function<void(const im::Message&)>;

    IMClient(boost::asio::io_context& ioc,
             const std::string& host, uint16_t port,
             ReceiveCallback onRecv);

    void Connect();

    // 发送任意消息（底层通用接口）
    void Send(const im::Message& msg);

    // 高级业务接口：注册、登录、聊天
    void RegisterUser(const std::string& username, const std::string& password);
    void LoginUser(const std::string& username, const std::string& password);
    void SendChat(const std::string& receiver, const std::string& content);

    // 获取当前登录的用户ID（可能为空）
    std::optional<std::string> GetCurrentUserId() const;

    void Close();

private:
    void AsyncReadLength();
    void AsyncReadBody(int32_t bodyLen);
    void OnMessageReceived(const im::Message& msg);
    void DoWrite();

    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver resolver_;
    std::string host_;
    uint16_t port_;
    ReceiveCallback onReceive_;

    std::array<char, 4> lengthBuffer_{};
    std::vector<char> bodyBuffer_;

    std::deque<std::string> sendQueue_;
    std::mutex sendMutex_;
    bool writing_ = false;

    // 客户端保存的序列号和当前用户ID
    int64_t seq_ = 0;
    std::optional<std::string> currentUserId_;
    mutable std::mutex userIdMutex_;
};