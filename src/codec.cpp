#include "codec.h"
#include <arpa/inet.h>
#include <spdlog/spdlog.h>

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
        spdlog::error("Failed to parse protobuf message");
        return std::nullopt;
    }
    return msg;
}