// im_client.cpp
#include "im_client.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

// ---------- Codec ----------
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

// ---------- IMClient ----------
IMClient::IMClient(boost::asio::io_context& ioc,
                   const std::string& host, uint16_t port,
                   ReceiveCallback onRecv)
    : socket_(ioc), resolver_(ioc)
    , host_(host), port_(port)
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

void IMClient::SendSingleMsg(const std::string& receiver, const std::string& content) {
    if (!currentUserId_) { std::cerr << "Not logged in\n"; return; }
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_SINGLE_MSG);
    msg.mutable_header()->set_seq(++seq_);
    im::ChatMessage chat;
    chat.set_sender(*currentUserId_);
    chat.set_receiver(receiver);
    chat.set_content(content);
    msg.set_body(chat.SerializeAsString());
    Send(msg);
}

void IMClient::SendGroupMsg(const std::string& group_id, const std::string& content) {
    if (!currentUserId_) { std::cerr << "Not logged in\n"; return; }
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

void IMClient::GetContacts() {
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_GET_CONTACTS_REQ);  // 需在 proto 中定义
    msg.mutable_header()->set_seq(++seq_);
    im::ContactRequest con;
    con.set_user_id(currentUserId_.value());
    msg.set_body(con.SerializeAsString());
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

void IMClient::DeleteFriendReq(const std::string& peer_id){
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_DELETE_FRIEND_REQ);
    msg.mutable_header()->set_seq(++seq_);

    im::DeleteFriendReq req;
    req.set_peer_id(peer_id);
    msg.set_body(req.SerializeAsString());
    Send(msg);
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

void IMClient::SendResponeToFriendReqsReq(const std::string& user_id, 
                const std::string& peer_id, int32_t status){
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_RESPONE_TO_FRIEND_REQS_REQ);
    msg.mutable_header()->set_seq(++seq_);

    im::ResponseToFriendReqsReq req;
    req.set_user_id(user_id);
    req.set_peer_id(peer_id);
    req.set_status(status);
    msg.set_body(req.SerializeAsString());
    Send(msg);
}

void IMClient::SendGetFriendReqsReq(){
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_GET_FRIEND_REQS_REQ);
    msg.mutable_header()->set_seq(++seq_);

    Send(msg);
}


void IMClient::SendAddFriendReq(const std::string& target_name, const std::string& sender_name, const std::string& m_msg){
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_ADD_FRIEND_REQ);
    msg.mutable_header()->set_seq(++seq_);
    im::AddFriendRequest req;
    req.set_from_user_id(*GetCurrentUserId());
    req.set_from_user_name(sender_name);
    req.set_to_user_name(target_name);
    req.set_message(m_msg);
    msg.set_body(req.SerializeAsString());
    Send(msg);
}

void IMClient::Disconnect(){
    if(!currentUserId_){ std::cerr << "Not logged in\n"; return;}
    std::cout<<"Disconnecting..."<<std::endl;
    im::Message msg;
    msg.mutable_header()->set_cmd(im::CMD_QUIT_REQ);
    msg.mutable_header()->set_seq(++seq_);
    msg.set_body(currentUserId_.value());
    Send(msg);
    currentUserId_=std::nullopt;
    std::cout<<"Disconnected"<<std::endl;
}

void IMClient::Close() {
    boost::system::error_code ec;
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}