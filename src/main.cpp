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
            if (resp.status() == 0) {
                std::cout << "[Register] Success" << std::endl;
            } else if (resp.status() == 2) {
                std::cout << "[Register] Failed: username already exists" << std::endl;
            } else {
                std::cout << "[Register] Failed with status " << resp.status() << std::endl;
            }
        }
    } else if (cmd == im::CMD_LOGIN_RES) {
        im::LoginResponse resp;
        if (resp.ParseFromString(msg.body())) {
            if (resp.status() == 0) {
                std::cout << "[Login] Success, user_id = " << resp.user_id() << std::endl;
            } else if (resp.status() == 1) {
                std::cout << "[Login] Failed: wrong password" << std::endl;
            } else if (resp.status() == 2) {
                std::cout << "[Login] Failed: user not found" << std::endl;
            } else {
                std::cout << "[Login] Failed with status " << resp.status() << std::endl;
            }
        }
    } else if (cmd == im::CMD_CHAT_RES) {
        std::cout << "[System] Message delivered, status = " << msg.header().status() << std::endl;
    } else if (cmd == im::CMD_CHAT_REQ) {
        im::ChatMessage chat;
        if (chat.ParseFromString(msg.body())) {
            std::cout << "[From " << chat.sender() << "] " << chat.content() << std::endl;
        }
    } else if (cmd == im::CMD_HEARTBEAT) {
        // ignore
    } else {
        std::cout << "[Unknown command: " << cmd << "]" << std::endl;
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
            try {
                ioc.run();
            } catch (const std::exception& e) {
                std::cerr << "IO thread exception: " << e.what() << std::endl;
            }
        });

        std::cout << "Commands:\n"
                  << "  register <username> <password>\n"
                  << "  login <username> <password>\n"
                  << "  send <receiver_id> <message>\n"
                  << "  quit\n";

        std::string line;
        while (g_running && std::getline(std::cin, line)) {
            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;
            if (cmd == "register") {
                std::string username, password;
                iss >> username >> password;
                if (username.empty() || password.empty()) {
                    std::cout << "Usage: register <username> <password>\n";
                } else {
                    client->RegisterUser(username, password);
                }
            } else if (cmd == "login") {
                std::string username, password;
                iss >> username >> password;
                if (username.empty() || password.empty()) {
                    std::cout << "Usage: login <username> <password>\n";
                } else {
                    client->LoginUser(username, password);
                }
            } else if (cmd == "send") {
                std::string receiver, content;
                iss >> receiver;
                std::getline(iss, content);
                if (receiver.empty() || content.empty()) {
                    std::cout << "Usage: send <receiver_id> <message>\n";
                } else {
                    // 去掉前导空格
                    if (content[0] == ' ') content.erase(0, 1);
                    client->SendChat(receiver, content);
                }
            } else if (cmd == "quit") {
                g_running = false;
                break;
            } else {
                std::cout << "Unknown command. Try register/login/send/quit\n";
            }
        }

        client->Close();
        ioc.stop();
        if (io_thread.joinable())
            io_thread.join();
        std::cout << "Client terminated.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}