//main.cpp
#include <atomic>
#include <boost/asio.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <filesystem>

#include "im_client.h"

std::atomic<bool> g_running{true};

void InitCombinedLogger() {
    std::filesystem::path current_p = std::filesystem::current_path();
    current_p=current_p.parent_path();
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    std::string log_path=current_p.string()+"/logs/im_server.log";
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path, false);

    std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
    auto combined_logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());
    
    // 🌟 1. 设置最低日志级别（建议设置为 debug 或 info）
    combined_logger->set_level(spdlog::level::debug);
    
    // 🌟 2. 设置日志格式：[时间] [日志级别] [线程ID] 内容
    combined_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");

    spdlog::set_default_logger(combined_logger);
    spdlog::flush_every(std::chrono::seconds(1));
    // 🌟 3. 遇到 error 级别的日志时立即刷新磁盘，防止程序挂掉丢失关键错误日志
    spdlog::flush_on(spdlog::level::err);
}

void OnReceive(const im::Message& msg) {
    int cmd = msg.header().cmd();
    
    if (cmd == im::CMD_REGISTER_RES) {
        im::RegisterResponse resp;
        if (resp.ParseFromString(msg.body())) {
            if (resp.status() == 0) {
                spdlog::info("[Register] Success");
            } else if (resp.status() == 2) {
                spdlog::warn("[Register] Failed: Username already exists");
            } else {
                spdlog::warn("[Register] Failed: status={}", resp.status());
            }
        }
    } else if (cmd == im::CMD_LOGIN_RES) {
        im::LoginResponse resp;
        if (resp.ParseFromString(msg.body())) {
            if (resp.status() == 0) {
                spdlog::info("[Login] Success, user_id={}", resp.user_id());
            } else if (resp.status() == 1) {
                spdlog::warn("[Login] Failed: Wrong password");
            } else if (resp.status() == 2) {
                spdlog::warn("[Login] Failed: User not found");
            } else {
                spdlog::warn("[Login] Failed: status={}", resp.status());
            }
        }
    } else if (cmd == im::CMD_GET_HISTORY_RES) {
        im::HistoryResponse resp;
        if (resp.ParseFromString(msg.body())) {
            spdlog::info("=== History Message Start ===");
            for (const auto& m : resp.messages()) {
                spdlog::info("[{}] {}: {}", m.msg_id(), m.sender(), m.content());
            }
            spdlog::info("=== History Message End ===");
        }
    } else if (cmd == im::CMD_CLEAR_UNREAD_RES) {
        spdlog::info("[System] Unread cleared");
    } else if (cmd == im::CMD_SINGLE_MSG || cmd == im::CMD_GROUP_MSG) {
        im::ChatMessage chat;
        if (chat.ParseFromString(msg.body())) {
            spdlog::info("[Message From {}]: {}", chat.sender(), chat.content());
        }
    } else if (cmd == im::CMD_HEARTBEAT) {
        spdlog::debug("Received heartbeat response");
        
    } else {
        spdlog::warn("[Unknown command ID: {}]", cmd);
    }
}

void PrintMenu() {
    spdlog::info("------------------ Command List ------------------");
    spdlog::info("  register <username> <password>");
    spdlog::info("  login <username> <password>");
    spdlog::info("  send <receiver_id> <message>");
    spdlog::info("  gsend <group_id> <message>");
    spdlog::info("  history <peer_id> <is_group(0/1)> <start> <count>");
    spdlog::info("  clear <peer_id> <is_group(0/1)>");
    spdlog::info("  disconnect");
    spdlog::info("  quit");
    spdlog::info("--------------------------------------------------");
}

int main(int argc, char* argv[]) {
    try {
        if (argc != 3) {
            spdlog::error("Usage: {} <host> <port>", argv[0]);
            return 1;
        }

        std::string host = argv[1];
        uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
        InitCombinedLogger();
        boost::asio::io_context ioc;
        auto client = std::make_shared<IMClient>(ioc, host, port, OnReceive);
        client->Connect();

        std::thread io_thread([&ioc] {
            try { 
                ioc.run(); 
            } catch (const std::exception& e) {
                spdlog::error("IO Thread error: {}", e.what());
            }
        });

        PrintMenu();

        std::string line;
        while (g_running && std::getline(std::cin, line)) {
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;

            if (cmd.empty()) continue;

            if (cmd == "register") {
                std::string u, p;
                if (iss >> u >> p) {
                    client->RegisterUser(u, p);
                } else {
                    spdlog::warn("Usage: register <username> <password>");
                }
            } else if (cmd == "login") {
                std::string u, p;
                if (iss >> u >> p) {
                    client->LoginUser(u, p);
                } else {
                    spdlog::warn("Usage: login <username> <password>");
                }
            } else if (cmd == "send") {
                std::string receiver, content;
                iss >> receiver;
                std::getline(iss, content);
                if (!receiver.empty() && !content.empty()) {
                    if (content[0] == ' ') content.erase(0, 1);
                    client->SendSingleMsg(receiver, content);
                } else {
                    spdlog::warn("Usage: send <receiver_id> <message>");
                }
            } else if (cmd == "gsend") {
                std::string group, content;
                iss >> group;
                std::getline(iss, content);
                if (!group.empty() && !content.empty()) {
                    if (content[0] == ' ') content.erase(0, 1);
                    client->SendGroupMsg(group, content);
                } else {
                    spdlog::warn("Usage: gsend <group_id> <message>");
                }
            } else if (cmd == "history") {
                std::string peer;
                int is_grp;
                int64_t start;
                int count;
                if (iss >> peer >> is_grp >> start >> count) {
                    client->GetHistory(peer, is_grp != 0, start, count);
                } else {
                    spdlog::warn("Usage: history <peer> <is_group(0/1)> <start> <count>");
                }
            } else if (cmd == "clear") {
                std::string peer;
                int is_grp;
                if (iss >> peer >> is_grp) {
                    client->ClearUnread(peer, is_grp != 0);
                } else {
                    spdlog::warn("Usage: clear <peer> <is_group(0/1)>");
                }
            } else if (cmd == "disconnect") {
                client->LogOutUser();
            } else if (cmd == "quit") {
                g_running = false;
                break;
            } else {
                spdlog::warn("Unknown command: {}", cmd);
            }
        }

        client->Close();
        ioc.stop();
        if (io_thread.joinable()) {
            io_thread.join();
        }
        spdlog::info("Client terminated safely.");
    } catch (const std::exception& e) {
        spdlog::critical("Unhandled exception: {}", e.what());
        return 1;
    }

    return 0;
}