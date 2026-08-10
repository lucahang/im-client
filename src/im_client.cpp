#include "im_client.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

// ---------- Codec 实现（同服务端） ----------
std::string Codec::Encode(const im::Message& msg) {
    std::string body = msg.SerializeAsString();
    int32_t netLen = htonl(static_cast<int32_t>(body.size()));
    std::string result;
    result.reserve(4 + body.size());
    result.append(reinterpret_cast<const char*>(&netLen), 4);
    result.append(body);
    return result;
}

std::optional<im::Message> Codec::Decode(const char* data, size_t len) {
    im::Message msg;
    if (!msg.ParseFromArray(data, static_cast<int>(len))) {
        return std::nullopt;
    }
    return msg;
}

// ---------- IMClient 实现 ----------
IMClient::IMClient(boost::asio::io_context& ioc,
                   const std::string& host, uint16_t port,
                   ReceiveCallback onRecv)
    : socket_(ioc)
    , resolver_(ioc)
    , host_(host)
    , port_(port)
    , onReceive_(std::move(onRecv)) {}

void IMClient::Connect() {
    auto self = shared_from_this();
    resolver_.async_resolve(host_, std::to_string(port_),
        [this, self](boost::system::error_code ec,
                     boost::asio::ip::tcp::resolver::results_type endpoints) {
            if (!ec) {
                boost::asio::async_connect(socket_, endpoints,
                    [this, self](boost::system::error_code ec,
                                 boost::asio::ip::tcp::endpoint) {
                        if (!ec) {
                            std::cout << "Connected to server." << std::endl;
                            AsyncReadLength();
                        } else {
                            std::cerr << "Connect failed: " << ec.message() << std::endl;
                        }
                    });
            } else {
                std::cerr << "Resolve failed: " << ec.message() << std::endl;
            }
        });
}

void IMClient::Send(const im::Message& msg) {
    std::string data = Codec::Encode(msg);
    {
        std::lock_guard<std::mutex> lock(sendMutex_);
        sendQueue_.push_back(std::move(data));
        if (!writing_) {
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

void IMClient::SendChat(const std::string& receiver, const std::string& content) {
    if (!currentUserId_) {
        std::cerr << "Error: Not logged in. Please login first.\n";
        return;
    }
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_CHAT_REQ);
    msg.mutable_header()->set_seq(++seq_);
    im::ChatMessage chat;
    chat.set_sender(*currentUserId_);   // 自动填充当前用户ID
    chat.set_receiver(receiver);
    chat.set_content(content);
    msg.set_body(chat.SerializeAsString());
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
                if (ec != boost::asio::error::eof)
                    std::cerr << "Read error: " << ec.message() << std::endl;
                Close();
                return;
            }
            int32_t bodyLen = 0;
            std::memcpy(&bodyLen, lengthBuffer_.data(), 4);
            bodyLen = ntohl(bodyLen);
            if (bodyLen <= 0 || bodyLen > 1024 * 1024) {
                Close();
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
                Close();
                return;
            }
            auto msg = Codec::Decode(bodyBuffer_.data(), bodyBuffer_.size());
            if (msg) {
                OnMessageReceived(*msg);
            }
            AsyncReadLength();
        });
}

void IMClient::OnMessageReceived(const im::Message& msg) {
    // 特殊处理登录响应，保存用户ID
    if (msg.header().cmd() == im::CMD_LOGIN_RES && msg.header().status() == 0) {
        im::LoginResponse resp;
        if (resp.ParseFromString(msg.body())) {
            std::lock_guard<std::mutex> lock(userIdMutex_);
            currentUserId_ = resp.user_id();
        }
    }
    // 交给用户回调
    if (onReceive_) {
        onReceive_(msg);
    }
}

void IMClient::DoWrite() {
    std::string data;
    {
        std::lock_guard<std::mutex> lock(sendMutex_);
        if (sendQueue_.empty()) {
            writing_ = false;
            return;
        }
        data = std::move(sendQueue_.front());
        sendQueue_.pop_front();
    }
    auto self = shared_from_this();
    boost::asio::async_write(socket_, boost::asio::buffer(data),
        [this, self](boost::system::error_code ec, std::size_t /*len*/) {
            if (ec) {
                Close();
                return;
            }
            DoWrite();
        });
}

void IMClient::Close() {
    boost::system::error_code ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}