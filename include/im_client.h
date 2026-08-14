#pragma once

#include <boost/asio.hpp>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "codec.h"
#include "message.pb.h"

class IMClient : public std::enable_shared_from_this<IMClient> {
public:
    using ReceiveCallback = std::function<void(const im::Message&)>;

    IMClient(boost::asio::io_context& ioc,
             const std::string& host, uint16_t port,
             ReceiveCallback onRecv);
    ~IMClient();

    void Connect();
    void Send(const im::Message& msg);

    // 业务接口
    void RegisterUser(const std::string& username, const std::string& password);
    void LoginUser(const std::string& username, const std::string& password);
    void SendSingleMsg(const std::string& receiver, const std::string& content);
    void SendGroupMsg(const std::string& group_id, const std::string& content);
    void GetHistory(const std::string& peer_id, bool is_group, int64_t start, int32_t count);
    void ClearUnread(const std::string& peer_id, bool is_group);
    void LogOutUser();
    void Close();

    std::optional<std::string> GetCurrentUserId() const;

private:
    void DoConnect(const boost::asio::ip::tcp::resolver::results_type& endpoints);
    void ScheduleReconnect();
    
    // 心跳控制
    void StartHeartbeat();
    void StopHeartbeat();
    void SendHeartbeat();
    void HandleHeartbeatResponse(const im::Message& msg);

    void AsyncReadLength();
    void AsyncReadBody(int32_t bodyLen);
    void OnMessageReceived(const im::Message& msg);
    void DoWrite();
    void OnConnectionError(const std::string& reason);

    boost::asio::io_context& ioc_;
    boost::asio::ip::tcp::socket socket_;
    boost::asio::ip::tcp::resolver resolver_;
    
    // 心跳与重连定时器
    boost::asio::steady_timer heartbeatTimer_;
    boost::asio::steady_timer reconnectTimer_;
    
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

    bool isConnected_ = false;
    bool isManualClosed_ = false; // 是否为用户主动断开/退出
    
    // 常量配置
    static constexpr int kHeartbeatIntervalSec = 5;  // 心跳间隔5秒
    static constexpr int kReconnectIntervalSec = 3;  // 重连间隔3秒

    std::chrono::steady_clock::time_point lastHeartbeatAckTime_;
    
    // 心跳超时门限（例如 15 秒内没收到服务端心跳回应，认定连接假死）
    static constexpr int kHeartbeatTimeoutSec = 15;
};