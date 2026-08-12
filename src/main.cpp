//main.cpp
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <atomic>
#include <thread>
#include <sstream>

#include "im_client.h"

std::atomic<bool> g_running{true};

void OnReceive(const im::Message& msg) {
    int cmd = msg.header().cmd();
    if (cmd == im::CMD_REGISTER_RES) {
        im::RegisterResponse resp;
        if (resp.ParseFromString(msg.body())) {
            if (resp.status() == 0) std::cout << "[Register] Success" << std::endl;
            else if (resp.status() == 2) std::cout << "[Register] Failed: username exists" << std::endl;
            else std::cout << "[Register] Failed status=" << resp.status() << std::endl;
        }
    } else if (cmd == im::CMD_LOGIN_RES) {
        im::LoginResponse resp;
        if (resp.ParseFromString(msg.body())) {
            if (resp.status() == 0) std::cout << "[Login] Success, user_id=" << resp.user_id() << std::endl;
            else if (resp.status() == 1) std::cout << "[Login] Wrong password" << std::endl;
            else if (resp.status() == 2) std::cout << "[Login] User not found" << std::endl;
            else std::cout << "[Login] Failed status=" << resp.status() << std::endl;
        }
    } /*else if (cmd == im::CMD_CHAT_RES) {
        std::cout << "[System] Delivered status=" << msg.header().status() << std::endl;
    } */
    else if (cmd == im::CMD_GET_HISTORY_RES) {
        im::HistoryResponse resp;
        if (resp.ParseFromString(msg.body())) {
            std::cout << "=== History ===" << std::endl;
            for (auto& m : resp.messages()) {
                std::cout << "[" << m.msg_id() << "] " << m.sender() << ": " << m.content() << std::endl;
            }
            std::cout << "===============" << std::endl;
        }
    } else if (cmd == im::CMD_CLEAR_UNREAD_RES) {
        std::cout << "[System] Unread cleared" << std::endl;
    } else if (cmd == im::CMD_SINGLE_MSG || cmd == im::CMD_GROUP_MSG) {
        // 收到的聊天消息（转发）
        im::ChatMessage chat;
        if (chat.ParseFromString(msg.body())) {
            std::cout << "[From " << chat.sender() << "] " << chat.content() << std::endl;
        }
    } else if (cmd == im::CMD_HEARTBEAT) {
        
    } else {
        std::cout << "[Unknown command " << cmd << "]" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cerr << "Usage: " << argv[0] << " <host> <port>\n";
            return 1;
        }
        std::string host = argv[1];
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

        boost::asio::io_context ioc;
        auto client = std::make_shared<IMClient>(ioc, host, port, OnReceive);
        client->Connect();

        std::thread io_thread([&ioc] {
            try { ioc.run(); } catch (...) {}
        });

        std::cout << "Commands:\n"
                  << "  register <username> <password>\n"
                  << "  login <username> <password>\n"
                  << "  send <receiver_id> <message>\n"
                  << "  gsend <group_id> <message>\n"
                  << "  history <peer_id> <is_group(0/1)> <start> <count>\n"
                  << "  clear <peer_id> <is_group(0/1)>\n"
                  << "  disconnect\n"
                  << "  quit\n";

        std::string line;
        while (g_running && std::getline(std::cin, line)) {
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;
            if (cmd == "register") {
                std::string u, p;
                iss >> u >> p;
                if (!u.empty() && !p.empty()) client->RegisterUser(u, p);
                else std::cout << "Usage: register <username> <password>\n";
            } else if (cmd == "login") {
                std::string u, p;
                iss >> u >> p;
                if (!u.empty() && !p.empty()) client->LoginUser(u, p);
                else std::cout << "Usage: login <username> <password>\n";
            } else if (cmd == "send") {
                std::string receiver, content;
                iss >> receiver;
                std::getline(iss, content);
                if (!receiver.empty() && !content.empty()) {
                    if (content[0] == ' ') content.erase(0,1);
                    client->SendSingleMsg(receiver, content);
                } else std::cout << "Usage: send <receiver_id> <message>\n";
            } else if (cmd == "gsend") {
                std::string group, content;
                iss >> group;
                std::getline(iss, content);
                if (!group.empty() && !content.empty()) {
                    if (content[0] == ' ') content.erase(0,1);
                    client->SendGroupMsg(group, content);
                } else std::cout << "Usage: gsend <group_id> <message>\n";
            } else if (cmd == "history") {
                std::string peer; int is_grp; int64_t start; int count;
                iss >> peer >> is_grp >> start >> count;
                if (!peer.empty()) client->GetHistory(peer, is_grp, start, count);
                else std::cout << "Usage: history <peer> <is_group(0/1)> <start> <count>\n";
            } else if (cmd == "clear") {
                std::string peer; int is_grp;
                iss >> peer >> is_grp;
                if (!peer.empty()) client->ClearUnread(peer, is_grp);
                else std::cout << "Usage: clear <peer> <is_group(0/1)>\n";
            } else if(cmd == "disconnect"){
                client->Disconnect();
            } else if (cmd == "quit") {
                g_running = false; break;
            } else {
                
                std::cout << "Unknown command.\n";
            }
        }

        client->Close();
        ioc.stop();
        if (io_thread.joinable()) io_thread.join();
        std::cout << "Client terminated.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}