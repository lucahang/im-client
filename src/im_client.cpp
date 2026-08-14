#include "im_client.h"
#include <arpa/inet.h>
#include <spdlog/spdlog.h>
#include <cstring>

IMClient::IMClient(boost::asio::io_context& ioc,
                   const std::string& host, uint16_t port,
                   ReceiveCallback onRecv)
    : ioc_(ioc)
    , socket_(ioc)
    , resolver_(ioc)
    , heartbeatTimer_(ioc)
    , reconnectTimer_(ioc)
    , host_(host)
    , port_(port)
    , onReceive_(std::move(onRecv)) {}

IMClient::~IMClient() {
    Close();
}

void IMClient::Connect() {
    isManualClosed_ = false;
    auto self = shared_from_this();
    resolver_.async_resolve(host_, std::to_string(port_),
        [this, self](boost::system::error_code ec,
                     boost::asio::ip::tcp::resolver::results_type endpoints) {
            if (!ec) {
                DoConnect(endpoints);
            } else {
                spdlog::error("Resolve failed: {}", ec.message());
                OnConnectionError("Resolve failed");
            }
        });
}

void IMClient::DoConnect(const boost::asio::ip::tcp::resolver::results_type& endpoints) {
    auto self = shared_from_this();
    boost::asio::async_connect(socket_, endpoints,
        [this, self, endpoints](boost::system::error_code ec,
                                boost::asio::ip::tcp::endpoint) {
            if (!ec) {
                spdlog::info("Connected to server successfully.");
                isConnected_ = true;
                
                lastHeartbeatAckTime_ = std::chrono::steady_clock::now();

                // 启动读循环与心跳机制
                AsyncReadLength();
                StartHeartbeat();

                // 如果队列里有未发送的数据，继续写
                {
                    std::lock_guard<std::mutex> lock(sendMutex_);
                    if (!sendQueue_.empty() && !writing_) {
                        writing_ = true;
                        DoWrite();
                    }
                }
            } else {
                spdlog::error("Connect failed: {}", ec.message());
                OnConnectionError("Connect failed");
            }
        });
}

void IMClient::ScheduleReconnect() {
    if (isManualClosed_) {
        spdlog::info("Client was manually closed. Skip auto reconnect.");
        return;
    }

    spdlog::info("Will attempt to reconnect in {} seconds...", kReconnectIntervalSec);
    reconnectTimer_.expires_after(std::chrono::seconds(kReconnectIntervalSec));
    
    auto self = shared_from_this();
    reconnectTimer_.async_wait([this, self](boost::system::error_code ec) {
        if (!ec) {
            spdlog::info("Reconnecting to server...");
            Connect();
        }
    });
}

void IMClient::StartHeartbeat() {
    StopHeartbeat();
    heartbeatTimer_.expires_after(std::chrono::seconds(kHeartbeatIntervalSec));
    
    auto self = shared_from_this();
    heartbeatTimer_.async_wait([this, self](boost::system::error_code ec) {
        if (!ec && isConnected_) {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - lastHeartbeatAckTime_).count();

            // 检查心跳响应是否超时（比如超过 15 秒未收到 Response）
            if (duration > kHeartbeatTimeoutSec) {
                spdlog::error("[Heartbeat] Heartbeat timeout! No ack for {} seconds. Resetting connection...", duration);
                // 主动触发连接异常，进入断开重连流程
                OnConnectionError("Heartbeat timeout");
                return;
            }

            // 发送下一次心跳
            SendHeartbeat();
            StartHeartbeat(); // 循环下一次定时器
        }
    });
}

void IMClient::StopHeartbeat() {
    boost::system::error_code ec;
    heartbeatTimer_.cancel(ec);
}

void IMClient::SendHeartbeat() {
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_HEARTBEAT);
    msg.mutable_header()->set_seq(++seq_);
    
    spdlog::debug("[Heartbeat] Sending heartbeat ping...");
    Send(msg);
}

void IMClient::Send(const im::Message& msg) {
    std::string data = Codec::Encode(msg);
    {
        std::lock_guard<std::mutex> lock(sendMutex_);
        sendQueue_.push_back(std::move(data));
        if (isConnected_ && !writing_) {
            writing_ = true;
            boost::asio::post(socket_.get_executor(),
                [self = shared_from_this()] { self->DoWrite(); });
        }
    }
}

void IMClient::RegisterUser(const std::string& username, const std::string& password) {
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_REGISTER_REQ);
    msg.mutable_header()->set_seq(++seq_);
    
    im::RegisterRequest req;
    req.set_username(username);
    req.set_password(password);
    msg.set_body(req.SerializeAsString());
    
    Send(msg);
}

void IMClient::LoginUser(const std::string& username, const std::string& password) {
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_LOGIN_REQ);
    msg.mutable_header()->set_seq(++seq_);
    
    im::LoginRequest req;
    req.set_username(username);
    req.set_password(password);
    msg.set_body(req.SerializeAsString());
    
    Send(msg);
}

void IMClient::SendSingleMsg(const std::string& receiver, const std::string& content) {
    if (!currentUserId_) {
        spdlog::warn("Cannot send message: Not logged in");
        return;
    }
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_SINGLE_MSG);
    msg.mutable_header()->set_seq(++seq_);
    
    im::ChatMessage chat;
    chat.set_receiver(receiver);
    chat.set_content(content);
    msg.set_body(chat.SerializeAsString());
    
    Send(msg);
}

void IMClient::SendGroupMsg(const std::string& group_id, const std::string& content) {
    if (!currentUserId_) {
        spdlog::warn("Cannot send message: Not logged in");
        return;
    }
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_GROUP_MSG);
    msg.mutable_header()->set_seq(++seq_);
    
    im::ChatMessage chat;
    chat.set_group_id(group_id);
    chat.set_content(content);
    msg.set_body(chat.SerializeAsString());
    
    Send(msg);
}

void IMClient::GetHistory(const std::string& peer_id, bool is_group, int64_t start, int32_t count) {
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_GET_HISTORY_REQ);
    msg.mutable_header()->set_seq(++seq_);
    
    im::HistoryRequest req;
    req.set_peer_id(peer_id);
    req.set_is_group(is_group);
    req.set_start(start);
    req.set_count(count);
    msg.set_body(req.SerializeAsString());
    
    Send(msg);
}

void IMClient::ClearUnread(const std::string& peer_id, bool is_group) {
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_CLEAR_UNREAD_REQ);
    msg.mutable_header()->set_seq(++seq_);
    
    im::ClearUnreadRequest req;
    req.set_peer_id(peer_id);
    req.set_is_group(is_group);
    msg.set_body(req.SerializeAsString());
    
    Send(msg);
}

std::optional<std::string> IMClient::GetCurrentUserId() const {
    std::lock_guard<std::mutex> lock(userIdMutex_);
    return currentUserId_;
}

void IMClient::AsyncReadLength() {
    auto self = shared_from_this();
    boost::asio::async_read(socket_, boost::asio::buffer(lengthBuffer_),
        [this, self](boost::system::error_code ec, std::size_t /*len*/) {
            if (ec) {
                if (ec != boost::asio::error::eof) {
                    spdlog::error("Read length error: {}", ec.message());
                } else {
                    spdlog::warn("Server disconnected (EOF).");
                }
                OnConnectionError("Read length failed");
                return;
            }
            int32_t bodyLen = 0;
            std::memcpy(&bodyLen, lengthBuffer_.data(), 4);
            bodyLen = ntohl(bodyLen);
            
            if (bodyLen <= 0 || bodyLen > 1024 * 1024) {
                spdlog::error("Invalid body length: {}", bodyLen);
                OnConnectionError("Invalid body length");
                return;
            }
            AsyncReadBody(bodyLen);
        });
}

void IMClient::AsyncReadBody(int32_t bodyLen) {
    auto self = shared_from_this();
    bodyBuffer_.resize(bodyLen);
    
    boost::asio::async_read(socket_, boost::asio::buffer(bodyBuffer_),
        [this, self](boost::system::error_code ec, std::size_t /*len*/) {
            if (ec) {
                spdlog::error("Read body error: {}", ec.message());
                OnConnectionError("Read body failed");
                return;
            }
            auto msg = Codec::Decode(bodyBuffer_.data(), bodyBuffer_.size());
            if (msg) {
                OnMessageReceived(*msg);
            }
            AsyncReadLength();
        });
}

void IMClient::HandleHeartbeatResponse(const im::Message& msg) {
    // 更新最后一次收到心跳回应的时间戳
    lastHeartbeatAckTime_ = std::chrono::steady_clock::now();
    spdlog::trace("[Heartbeat] Received heartbeat ack from server.");
}

void IMClient::OnMessageReceived(const im::Message& msg) {
    if (msg.header().cmd() == im::CMD_HEARTBEAT) {
        HandleHeartbeatResponse(msg);
        return; // 处理完毕直接返回，不需要回调给上层业务代码
    }
    if (msg.header().cmd() == im::CMD_LOGIN_RES && msg.header().status() == 0) {
        im::LoginResponse resp;
        if (resp.ParseFromString(msg.body())) {
            std::lock_guard<std::mutex> lock(userIdMutex_);
            currentUserId_ = resp.user_id();
        }
    }
    
    if (onReceive_) {
        onReceive_(msg);
    }
}

void IMClient::DoWrite() {
    std::string data;
    {
        std::lock_guard<std::mutex> lock(sendMutex_);
        if (sendQueue_.empty() || !isConnected_) {
            writing_ = false;
            return;
        }
        data = sendQueue_.front();
    }
    
    auto self = shared_from_this();
    boost::asio::async_write(socket_, boost::asio::buffer(data),
        [this, self](boost::system::error_code ec, std::size_t /*len*/) {
            if (ec) {
                spdlog::error("Write error: {}", ec.message());
                OnConnectionError("Write failed");
                return;
            }
            
            {
                std::lock_guard<std::mutex> lock(sendMutex_);
                if (!sendQueue_.empty()) {
                    sendQueue_.pop_front();
                }
            }
            DoWrite();
        });
}

void IMClient::OnConnectionError(const std::string& reason) {
    if (!isConnected_ && socket_.is_open() == false) {
        return; // 防止重复触发重连逻辑
    }
    spdlog::warn("Connection lost, reason: {}. Resetting socket...", reason);
    
    isConnected_ = false;
    StopHeartbeat();

    boost::system::error_code ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);

    {
        std::lock_guard<std::mutex> lock(sendMutex_);
        writing_ = false;
    }

    // 触发重连
    ScheduleReconnect();
}

void IMClient::LogOutUser() {
    if (!currentUserId_) {
        spdlog::warn("Cannot logout: Not logged in");
        return;
    }
    spdlog::info("User logging out...");
    
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_QUIT_REQ);
    msg.mutable_header()->set_seq(++seq_);
    msg.set_body(currentUserId_.value());
    
    Send(msg);

    // 清除本地登录状态
    {
        std::lock_guard<std::mutex> lock(userIdMutex_);
        currentUserId_ = std::nullopt;
    }
    spdlog::info("Logged out successfully.");
}

void IMClient::Close() {
    isManualClosed_ = true;
    isConnected_ = false;
    
    StopHeartbeat();
    
    boost::system::error_code ec;
    reconnectTimer_.cancel(ec);
    resolver_.cancel();
    
    if (socket_.is_open()) {
        socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        socket_.close(ec);
    }
}