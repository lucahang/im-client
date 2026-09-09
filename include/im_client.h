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
    void Send(const im::Message& msg);

    // 高级接口
    void RegisterUser(const std::string& username, const std::string& password);
    void LoginUser(const std::string& username, const std::string& password);
    void SendSingleMsg(const std::string& receiver, const std::string& content);
    void SendGroupMsg(const std::string& group_id, const std::string& content);
    void SendAddFriendReq(const std::string& target_name, const std::string& msg);
    void GetHistory(const std::string& peer_id, bool is_group, int64_t start, int32_t count);
    void SendGetFriendReqsReq();
    void ClearUnread(const std::string& peer_id, bool is_group);
    void GetContacts();   // 新增：请求联系人列表
    void Disconnect();

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

    int64_t seq_ = 0;
    std::optional<std::string> currentUserId_;
    mutable std::mutex userIdMutex_;
};